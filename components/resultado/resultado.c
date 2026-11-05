
#include "resultado.h"
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

// 1. PROTOTIPOS DE FUNCIONES INTERNAS

static void MostrarFilaTablaResultado(const char *Metrica,
                                      const char *SinDma,
                                      const char *ConDma,
                                      const char *Diferencia);

static void MostrarFilaDecimalResultado(const char *Metrica,
                                        double SinDma,
                                        double ConDma,
                                        int Decimales);

static void MostrarFilaEnteraResultado(const char *Metrica,
                                       uint64_t SinDma,
                                       uint64_t ConDma);


// 2. MENSAJES INICIALES DEL SISTEMA

void MostrarTituloResultado(void)
{
    printf("\nEVALUACION ENERGETICA\n\n");
}

void MostrarInicializacionResultado(void)
{
    printf("Inicializacion del sistema...\n\n");
}

void MostrarEstabilizacionResultado(void)
{
    printf("Estabilizacion del sistema...\n\n");
}

void MostrarVerificacionSensorResultado(void)
{
    printf("Verificacion del sensor INA219...\n\n");
}


// 3. REPORTE PERIODICO CADA 5 SEGUNDOS

void MostrarReportePeriodicoResultado(const resultado_prueba_t *Resultado)
{
    if (Resultado == NULL)
    {
        return;
    }

    if (Resultado->Modo != MODO_SIN_DMA &&
        Resultado->Modo != MODO_CON_DMA)
    {
        return;
    }

    const char *NombreModo;

    if (Resultado->Modo == MODO_SIN_DMA)
    {
        NombreModo = "SIN DMA";
    }
    else
    {
        NombreModo = "CON DMA";
    }

    printf("\n");
    printf("PRUEBA %u/%u - ADC %s\n",
           (unsigned int)Resultado->Repeticion,
           (unsigned int)NUMERO_REPETICIONES,
           NombreModo);

    printf("Tiempo:              %.1f / %u s\n",
           Resultado->DuracionSegundos,
           (unsigned int)(DURACION_PRUEBA_MS / 1000U));

    printf("Voltaje:             %.3f V\n",
           Resultado->VoltajePromedio_V);

    printf("Corriente:           %.2f mA\n",
           Resultado->CorrientePromedio_mA);

    printf("Potencia:            %.2f mW\n",
           Resultado->PotenciaPromedio_mW);

    printf("Energia acumulada:   %.4f J\n\n",
           Resultado->EnergiaAcumulada_J);

    printf("Muestras validas:    %" PRIu64 "\n",
           Resultado->MuestrasValidas);

    printf("Muestras perdidas:   %" PRIu64 "\n",
           Resultado->MuestrasPerdidas);

    printf("Errores:             %" PRIu32 "\n",
           Resultado->Errores);

    printf("Frecuencia efectiva: %.2f Hz\n",
           Resultado->FrecuenciaEfectiva_Hz);

    if (Resultado->Modo == MODO_CON_DMA)
    {
        printf("Desbordamientos DMA: %" PRIu32 "\n",
               Resultado->DesbordamientosDma);
    }
    else
    {
        printf("Desbordamientos DMA: No aplica\n");
    }

    printf("\n");
}


// 4. MENSAJES DE FINALIZACION Y CONTINUACION

void MostrarPruebaFinalizadaResultado(void)
{
    printf("Prueba finalizada.\n\n");
}

void MostrarContinuacionResultado(void)
{
    printf("Continuando con la siguiente prueba...\n\n");
}


// 5. FUNCIONES INTERNAS PARA CONSTRUIR LA TABLA

static void MostrarFilaTablaResultado(const char *Metrica,
                                      const char *SinDma,
                                      const char *ConDma,
                                      const char *Diferencia)
{
    printf("%-40s | %-20s | %-20s | %s\n",
           Metrica,
           SinDma,
           ConDma,
           Diferencia);
}


static void MostrarFilaDecimalResultado(const char *Metrica,
                                        double SinDma,
                                        double ConDma,
                                        int Decimales)
{
    char TextoSinDma[32];
    char TextoConDma[32];
    char TextoDiferencia[32];

    snprintf(TextoSinDma,
             sizeof(TextoSinDma),
             "%.*f",
             Decimales,
             SinDma);

    snprintf(TextoConDma,
             sizeof(TextoConDma),
             "%.*f",
             Decimales,
             ConDma);

    snprintf(TextoDiferencia,
             sizeof(TextoDiferencia),
             "%+.*f",
             Decimales,
             ConDma - SinDma);

    MostrarFilaTablaResultado(Metrica,
                              TextoSinDma,
                              TextoConDma,
                              TextoDiferencia);
}


static void MostrarFilaEnteraResultado(const char *Metrica,
                                       uint64_t SinDma,
                                       uint64_t ConDma)
{
    char TextoSinDma[32];
    char TextoConDma[32];
    char TextoDiferencia[32];

    snprintf(TextoSinDma,
             sizeof(TextoSinDma),
             "%" PRIu64,
             SinDma);

    snprintf(TextoConDma,
             sizeof(TextoConDma),
             "%" PRIu64,
             ConDma);

    snprintf(TextoDiferencia,
             sizeof(TextoDiferencia),
             "%+" PRId64,
             (int64_t)ConDma - (int64_t)SinDma);

    MostrarFilaTablaResultado(Metrica,
                              TextoSinDma,
                              TextoConDma,
                              TextoDiferencia);
}


// 6. TABLA FINAL DE RESULTADOS COMPARATIVOS

void MostrarComparacionResultado(const comparacion_resultados_t *Comparacion)
{
    if (Comparacion == NULL)
    {
        return;
    }

    const resultado_prueba_t *SinDma = &Comparacion->SinDma;
    const resultado_prueba_t *ConDma = &Comparacion->ConDma;

    printf("\n\n");
    printf("11.1 Tabla de resultados comparativos\n\n");

    MostrarFilaTablaResultado("Metrica",
                              "Sin DMA",
                              "Con DMA",
                              "Diferencia / observaciones");

    printf("----------------------------------------------------------------------------------------------------------------------------\n");

    // Duracion efectiva
    MostrarFilaDecimalResultado("Duracion efectiva (s)",
                                SinDma->DuracionSegundos,
                                ConDma->DuracionSegundos,
                                2);

    // Voltaje promedio
    MostrarFilaDecimalResultado("Voltaje promedio (V)",
                                SinDma->VoltajePromedio_V,
                                ConDma->VoltajePromedio_V,
                                3);

    // Corriente promedio
    MostrarFilaDecimalResultado("Corriente promedio (mA)",
                                SinDma->CorrientePromedio_mA,
                                ConDma->CorrientePromedio_mA,
                                2);

    // Potencia promedio
    MostrarFilaDecimalResultado("Potencia promedio (mW)",
                                SinDma->PotenciaPromedio_mW,
                                ConDma->PotenciaPromedio_mW,
                                2);

    // Energia acumulada
    MostrarFilaDecimalResultado("Energia acumulada (J)",
                                SinDma->EnergiaAcumulada_J,
                                ConDma->EnergiaAcumulada_J,
                                4);

    // Cantidad de muestras validas
    MostrarFilaEnteraResultado("Muestras validas",
                               SinDma->MuestrasValidas,
                               ConDma->MuestrasValidas);

    // Muestras perdidas y errores
    char TextoErroresSinDma[48];
    char TextoErroresConDma[48];
    char TextoDiferenciaErrores[80];

    snprintf(TextoErroresSinDma,
             sizeof(TextoErroresSinDma),
             "%" PRIu64 " / %" PRIu32,
             SinDma->MuestrasPerdidas,
             SinDma->Errores);

    snprintf(TextoErroresConDma,
             sizeof(TextoErroresConDma),
             "%" PRIu64 " / %" PRIu32,
             ConDma->MuestrasPerdidas,
             ConDma->Errores);

    snprintf(TextoDiferenciaErrores,
             sizeof(TextoDiferenciaErrores),
             "Perdidas: %+" PRId64 ", errores: %+" PRId64,
             (int64_t)ConDma->MuestrasPerdidas -
             (int64_t)SinDma->MuestrasPerdidas,
             (int64_t)ConDma->Errores -
             (int64_t)SinDma->Errores);

    MostrarFilaTablaResultado("Muestras perdidas / errores",
                              TextoErroresSinDma,
                              TextoErroresConDma,
                              TextoDiferenciaErrores);

    // Tiempo de procesamiento
    MostrarFilaDecimalResultado("Tiempo de procesamiento por bloque (ms)",
                                SinDma->TiempoProcesamientoBloque_ms,
                                ConDma->TiempoProcesamientoBloque_ms,
                                4);

    // Desbordamientos DMA
    char TextoDesbordamientos[32];

    snprintf(TextoDesbordamientos,
             sizeof(TextoDesbordamientos),
             "%" PRIu32,
             ConDma->DesbordamientosDma);

    MostrarFilaTablaResultado("Desbordamientos DMA",
                              "No aplica",
                              TextoDesbordamientos,
                              "Solo aplica al metodo con DMA");

    // Porcentaje final de reduccion energetica
    printf("\n");

    if (Comparacion->PorcentajeValido)
    {
        printf("Reduccion porcentual de energia respecto a la prueba sin DMA: %.2f %%\n",
               Comparacion->ReduccionEnergeticaPorcentaje);
    }
    else
    {
        printf("Reduccion porcentual de energia: No calculable.\n");
    }

    printf("\n");
}
