
#include "control.h"
#include "config.h"
#include "adc_pot.h"
#include "adc_dma.h"
#include "sensor_ina.h"
#include "procesamiento.h"
#include "resultado.h"
#include "esp_timer.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


// 1. VARIABLES GLOBALES DEL MODULO

static modo_adquisicion_t ModoActual = MODO_SIN_DMA;

static int64_t ProximaMuestraSinDmaUs = 0;
static int64_t ProximaLecturaInaUs = 0;
static int64_t ProximoReporteUs = 0;

static uint16_t BufferMuestras[MUESTRAS_POR_BLOQUE];

static bool AdcInicializado = false;
static bool DmaEnMarcha = false;
static bool AdcEstabilizacionActivo = false;

static uint16_t UltimoPot = 0;
static bool PotDisponible = false;


// 2. INICIALIZACION DE LA ESTABILIZACION

bool IniciarEstabilizacionControl(void)
{
    if (AdcEstabilizacionActivo)
    {
        return true;
    }

    AdcEstabilizacionActivo = InicializarAdcPot();

    return AdcEstabilizacionActivo;
}


// 3. LECTURAS DURANTE LA ESTABILIZACION

void LeerEstabilizacionControl(uint32_t Segundo)
{
    medicion_ina_t Medicion = {0};

    uint16_t Pot = 0;

    bool Valido = LeerMedicionSensorIna(&Medicion);
    bool AdcValido = LeerMuestraAdcPot(&Pot);

    printf("Estabilizacion %lu s: V=%.3f V | I=%.1f mA | P=%.1f mW | POT=%s%u | INA219 %s\n",
           (unsigned long)Segundo,
           Medicion.Voltaje_V,
           Medicion.Corriente_mA,
           Medicion.Potencia_mW,
           AdcValido ? "" : "ERROR ",
           (unsigned int)Pot,
           Valido ? "valido" : "INVALIDO");
}


// 4. FINALIZACION DE LA ESTABILIZACION

void FinalizarEstabilizacionControl(void)
{
    if (AdcEstabilizacionActivo)
    {
        FinalizarAdcPot();
    }

    AdcEstabilizacionActivo = false;
}


// 5. VERIFICACION DE ALIMENTACION ENTRE PRUEBAS

void VerificarAlimentacionControl(void)
{
    medicion_ina_t Medicion = {0};

    if (LeerMedicionSensorIna(&Medicion))
    {
        printf("Verificacion entre pruebas: V=%.3f V | I=%.1f mA\n",
               Medicion.Voltaje_V,
               Medicion.Corriente_mA);
    }
    else
    {
        printf("Aviso: lectura INA219 invalida durante la pausa.\n");
    }
}


// 6. INICIO DE CADA PRUEBA

bool IniciarPruebaControl(modo_adquisicion_t Modo, uint8_t Repeticion)
{
    ModoActual = Modo;

    if (Modo == MODO_SIN_DMA)
    {
        if (!InicializarAdcPot())
        {
            return false;
        }
    }
    else
    {
        if (!InicializarAdcDma())
        {
            return false;
        }

        AdcInicializado = true;

        if (!IniciarAdcDma())
        {
            return false;
        }

        DmaEnMarcha = true;
    }

    AdcInicializado = true;

    if (!IniciarPruebaProcesamiento(Modo, Repeticion))
    {
        return false;
    }

    int64_t InicioUs = esp_timer_get_time();

    ProximaMuestraSinDmaUs = InicioUs;

    ProximaLecturaInaUs =
        InicioUs + (int64_t)PERIODO_LECTURA_INA219_MS * 1000;

    ProximoReporteUs =
        InicioUs + (int64_t)PERIODO_REPORTE_UART_MS * 1000;

    PotDisponible = false;
    UltimoPot = 0;

    printf("\n");
    printf("INICIO PRUEBA %u/%u - ADC %s\n",
           (unsigned int)Repeticion,
           (unsigned int)NUMERO_REPETICIONES,
           Modo == MODO_CON_DMA ? "CON DMA" : "SIN DMA");
    printf("Duracion: 120 s | Frecuencia efectiva: 10000 Hz\n");

    return true;
}


// 7. ADQUISICION Y PROCESAMIENTO DE MUESTRAS

bool AdquirirMuestrasControl(void)
{
    size_t Cantidad = 0;
    size_t Invalidas = 0;

    if (ModoActual == MODO_SIN_DMA)
    {
        // Lectura mediante ADC One-shot y sondeo activo.

        for (size_t i = 0; i < MUESTRAS_POR_BLOQUE; i++)
        {
            while (esp_timer_get_time() < ProximaMuestraSinDmaUs)
            {
                // Espera activa hasta el instante de muestreo.
            }

            uint16_t Lectura = 0;

            bool Ok = LeerMuestraAdcPot(&Lectura);

            ProximaMuestraSinDmaUs += PERIODO_MUESTREO_US;

            int64_t DespuesUs = esp_timer_get_time();

            if (DespuesUs > ProximaMuestraSinDmaUs)
            {
                int64_t RetrasoUs =
                    DespuesUs - ProximaMuestraSinDmaUs;

                ProximaMuestraSinDmaUs +=
                    ((RetrasoUs / PERIODO_MUESTREO_US) + 1) *
                    PERIODO_MUESTREO_US;
            }

            if (Ok)
            {
                BufferMuestras[Cantidad++] = Lectura;
            }
            else
            {
                RegistrarErrorProcesamiento();
            }
        }
    }
    else
    {
        // Lectura del ADC continuo mediante DMA.

        if (!LeerBloqueAdcDma(BufferMuestras,
                             MUESTRAS_POR_BLOQUE,
                             &Cantidad,
                             &Invalidas))
        {
            RegistrarErrorProcesamiento();
            return false;
        }

        if (Cantidad == 0)
        {
            RegistrarLecturaDmaVaciaProcesamiento();
        }

        ActualizarDesbordamientosProcesamiento(
            ConsultarDesbordamientosAdcDma()
        );
    }

    // Procesamiento estadistico de cada bloque.

    if (!ProcesarBloqueProcesamiento(BufferMuestras, Cantidad))
    {
        return false;
    }

    // Se obtiene una lectura representativa del potenciometro.

    if (Cantidad > 0)
    {
        uint64_t Suma = 0;

        for (size_t i = 0; i < Cantidad; i++)
        {
            Suma += BufferMuestras[i];
        }

        UltimoPot = (uint16_t)(Suma / Cantidad);
        PotDisponible = true;
    }

    // Lectura periodica del sensor INA219.

    if (esp_timer_get_time() >= ProximaLecturaInaUs)
    {
        RegistrarConsumoControl();

        ProximaLecturaInaUs +=
            (int64_t)PERIODO_LECTURA_INA219_MS * 1000;
    }

    // Impresion periodica por UART.

    if (esp_timer_get_time() >= ProximoReporteUs)
    {
        MostrarReporteControl();

        ProximoReporteUs +=
            (int64_t)PERIODO_REPORTE_UART_MS * 1000;
    }

    return true;
}


// 8. REGISTRO DE CONSUMO ENERGETICO

void RegistrarConsumoControl(void)
{
    medicion_ina_t Medicion;

    if (!LeerMedicionSensorIna(&Medicion))
    {
        RegistrarMedicionInvalidaInaProcesamiento();
        return;
    }

    (void)RegistrarMedicionProcesamiento(&Medicion);
}


// 9. REPORTE PERIODICO DE LA PRUEBA

void MostrarReporteControl(void)
{
    resultado_prueba_t Resumen;

    if (!ObtenerResumenProcesamiento(&Resumen))
    {
        return;
    }

    MostrarReportePeriodicoResultado(&Resumen);

    if (PotDisponible)
    {
        printf("Potenciometro ADC: %u/4095 \n\n",
               (unsigned int)UltimoPot);
    }
    else
    {
        printf("Potenciometro ADC: sin datos\n\n");
    }
}


// 10. REPORTE DEFINITIVO DE CADA PRUEBA

void MostrarResumenFinalControl(modo_adquisicion_t Modo,
                                uint8_t Repeticion)
{
    resultado_prueba_t Resumen;

    if (!ConsultarResultadoProcesamiento(Modo,
                                         Repeticion,
                                         &Resumen))
    {
        return;
    }

    // Se muestra el resultado con el formato periodico.

    MostrarReportePeriodicoResultado(&Resumen);

    if (PotDisponible)
    {
        printf("Potenciometro ADC: %u / 4095\n\n",
               (unsigned int)UltimoPot);
    }
    else
    {
        printf("Potenciometro ADC: sin datos\n\n");
    }
}


// 11. FINALIZACION DE LA PRUEBA

bool FinalizarPruebaControl(void)
{
    // Ultima lectura del INA219 antes de cerrar la prueba.

    RegistrarConsumoControl();

    // Guardar los resultados finales del procesamiento.

    bool Bien = FinalizarPruebaProcesamiento();

    if (ModoActual == MODO_CON_DMA)
    {
        if (DmaEnMarcha && !DetenerAdcDma())
        {
            Bien = false;
        }

        DmaEnMarcha = false;

        if (!FinalizarAdcDma())
        {
            Bien = false;
        }
    }
    else
    {
        if (!FinalizarAdcPot())
        {
            Bien = false;
        }
    }

    AdcInicializado = false;

    return Bien;
}


// 12. LIBERACION DE LOS RECURSOS ADC

void LiberarAdquisicionControl(void)
{
    if (!AdcInicializado)
    {
        return;
    }

    if (ModoActual == MODO_CON_DMA)
    {
        if (DmaEnMarcha)
        {
            DetenerAdcDma();
        }

        FinalizarAdcDma();

        DmaEnMarcha = false;
    }
    else
    {
        FinalizarAdcPot();
    }

    AdcInicializado = false;
}