#include "procesamiento.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include "esp_timer.h"

typedef struct {
    bool Activa;
    int64_t InicioUs;
    int64_t UltimaMedicionValidaUs;
    double SumaVoltaje, SumaCorriente, SumaPotencia, Energia_J;
    uint64_t SumaAdc, SumaCuadradosAdc;
    uint32_t NumeroMedicionesValidas, NumeroMedicionesInvalidas;
    uint32_t Bloques;
    int64_t TiempoProcesamientoUs;
    resultado_prueba_t Resultado;
} estado_t;

static estado_t Estado;
static resultado_prueba_t Resultados[2][NUMERO_REPETICIONES];
static bool Disponibles[2][NUMERO_REPETICIONES];
static int64_t FinPruebaUs = 0;

void InicializarProcesamiento(void)
{
    memset(&Estado, 0, sizeof(Estado));
    memset(Resultados, 0, sizeof(Resultados));
    memset(Disponibles, 0, sizeof(Disponibles));
}

bool IniciarPruebaProcesamiento(modo_adquisicion_t Modo, uint8_t Repeticion)
{
    if (Estado.Activa || Repeticion < 1 || Repeticion > NUMERO_REPETICIONES ||
        (Modo != MODO_SIN_DMA && Modo != MODO_CON_DMA)) return false;
    memset(&Estado, 0, sizeof(Estado));
    Estado.Resultado.Modo = Modo;
    Estado.Resultado.Repeticion = Repeticion;
    Estado.Resultado.MinimoAdc = VALOR_MAXIMO_ADC;
    Estado.InicioUs = esp_timer_get_time();
    FinPruebaUs = Estado.InicioUs + (int64_t)DURACION_PRUEBA_MS * 1000;
    Estado.UltimaMedicionValidaUs = Estado.InicioUs;
    Estado.Activa = true;
    return true;
}

// Estadisticas de bloque: media, RMS, minimo y maximo, con el mismo procedimiento
// de suma y suma de cuadrados utilizado en el programa de referencia.
bool ProcesarBloqueProcesamiento(const uint16_t *Muestras, size_t Cantidad)
{
    if (!Estado.Activa || !Muestras) return false;
    int64_t Inicio = esp_timer_get_time();
    uint64_t Suma = 0, SumaCuadrados = 0;
    uint16_t Minimo = VALOR_MAXIMO_ADC, Maximo = 0;
    for (size_t i = 0; i < Cantidad; i++) {
        Suma += Muestras[i];
        SumaCuadrados += (uint64_t)Muestras[i] * Muestras[i];
        if (Muestras[i] < Minimo) Minimo = Muestras[i];
        if (Muestras[i] > Maximo) Maximo = Muestras[i];
    }
    if (Cantidad > 0) {
        Estado.SumaAdc += Suma;
        Estado.SumaCuadradosAdc += SumaCuadrados;
        if (Minimo < Estado.Resultado.MinimoAdc) Estado.Resultado.MinimoAdc = Minimo;
        if (Maximo > Estado.Resultado.MaximoAdc) Estado.Resultado.MaximoAdc = Maximo;
        Estado.Resultado.MuestrasValidas += Cantidad;
        Estado.Bloques++;
    }
    Estado.TiempoProcesamientoUs += esp_timer_get_time() - Inicio;
    return true;
}

bool RegistrarMedicionProcesamiento(const medicion_ina_t *Medicion)
{
    if (!Estado.Activa || !Medicion) return false;
    int64_t AhoraUs = esp_timer_get_time();
    if (!isfinite(Medicion->Voltaje_V) || !isfinite(Medicion->Corriente_mA) ||
        !isfinite(Medicion->Potencia_mW) || Medicion->Voltaje_V < 1.0f ||
        Medicion->Voltaje_V > 26.0f || Medicion->Corriente_mA < 0.0f) {
        Estado.NumeroMedicionesInvalidas++;
        return false;
    }
    // Integracion por rectangulos: P actual por el tiempo desde la ultima
    // medicion valida, como en el modulo de referencia.
    double DeltaSegundos = (double)(AhoraUs - Estado.UltimaMedicionValidaUs) / 1000000.0;
    Estado.Energia_J += (Medicion->Potencia_mW / 1000.0) * DeltaSegundos;
    Estado.SumaVoltaje += Medicion->Voltaje_V;
    Estado.SumaCorriente += Medicion->Corriente_mA;
    Estado.SumaPotencia += Medicion->Potencia_mW;
    Estado.NumeroMedicionesValidas++;
    Estado.UltimaMedicionValidaUs = AhoraUs;
    return true;
}

void RegistrarMedicionInvalidaInaProcesamiento(void)
{
    if (Estado.Activa) Estado.NumeroMedicionesInvalidas++;
}
void RegistrarLecturaDmaVaciaProcesamiento(void)
{
    if (Estado.Activa) Estado.Resultado.LecturasDmaVacias++;
}
void RegistrarMuestrasPerdidasProcesamiento(uint32_t Cantidad)
{
    if (Estado.Activa) Estado.Resultado.MuestrasPerdidas += Cantidad;
}
void RegistrarErrorProcesamiento(void)
{
    if (Estado.Activa) Estado.Resultado.Errores++;
}
void ActualizarDesbordamientosProcesamiento(uint32_t Cantidad)
{
    if (Estado.Activa && Estado.Resultado.Modo == MODO_CON_DMA)
        Estado.Resultado.DesbordamientosDma = Cantidad;
}
uint32_t ObtenerTiempoTranscurridoProcesamiento(void)
{
    if (!Estado.Activa) return 0;
    int64_t Delta = esp_timer_get_time() - Estado.InicioUs;
    return Delta < 0 ? 0 : (uint32_t)(Delta / 1000);
}
bool PruebaTerminadaProcesamiento(void)
{
    return Estado.Activa && esp_timer_get_time() >= FinPruebaUs;
}
bool ObtenerResumenProcesamiento(resultado_prueba_t *Resultado)
{
    if (!Estado.Activa || !Resultado) return false;
    *Resultado = Estado.Resultado;
    double Segundos = (double)(esp_timer_get_time() - Estado.InicioUs) / 1000000.0;
    Resultado->DuracionSegundos = Segundos;
    if (Estado.NumeroMedicionesValidas) {
        double N = Estado.NumeroMedicionesValidas;
        Resultado->VoltajePromedio_V = Estado.SumaVoltaje / N;
        Resultado->CorrientePromedio_mA = Estado.SumaCorriente / N;
        Resultado->PotenciaPromedio_mW = Estado.SumaPotencia / N;
    }
    Resultado->EnergiaAcumulada_J = Estado.Energia_J;
    Resultado->MedicionesInaValidas = Estado.NumeroMedicionesValidas;
    Resultado->MedicionesInaInvalidas = Estado.NumeroMedicionesInvalidas;
    Resultado->BloquesProcesados = Estado.Bloques;
    if (Resultado->MuestrasValidas) {
        Resultado->MediaAdc = (double)Estado.SumaAdc / Resultado->MuestrasValidas;
        Resultado->ValorRmsAdc = sqrt((double)Estado.SumaCuadradosAdc / Resultado->MuestrasValidas);
    }
    if (Estado.Bloques)
        Resultado->TiempoProcesamientoBloque_ms = (double)Estado.TiempoProcesamientoUs / (1000.0 * Estado.Bloques);
    if (Segundos > 0.0) {
        Resultado->FrecuenciaEfectiva_Hz = Resultado->MuestrasValidas / Segundos;
        // Solo en el resumen final se calcula el numero total de posiciones
        // teoricas que no produjeron muestras utiles, como en la referencia.
        if (PruebaTerminadaProcesamiento()) {
            uint64_t Esperadas = (uint64_t)(Segundos * FRECUENCIA_MUESTREO_HZ);
            Resultado->MuestrasPerdidas = Esperadas > Resultado->MuestrasValidas
                ? Esperadas - Resultado->MuestrasValidas : 0;
        }
    }
    return true;
}
bool FinalizarPruebaProcesamiento(void)
{
    if (!PruebaTerminadaProcesamiento() || !Estado.NumeroMedicionesValidas) return false;
    resultado_prueba_t Final;
    if (!ObtenerResumenProcesamiento(&Final)) return false;
    Resultados[Final.Modo][Final.Repeticion - 1] = Final;
    Disponibles[Final.Modo][Final.Repeticion - 1] = true;
    Estado.Activa = false;
    return true;
}
bool ConsultarResultadoProcesamiento(modo_adquisicion_t Modo, uint8_t Repeticion,
                                    resultado_prueba_t *Resultado)
{
    if (!Resultado || Repeticion == 0 || Repeticion > NUMERO_REPETICIONES ||
        (Modo != MODO_SIN_DMA && Modo != MODO_CON_DMA) || !Disponibles[Modo][Repeticion - 1]) return false;
    *Resultado = Resultados[Modo][Repeticion - 1];
    return true;
}
static bool PromediarResultados(modo_adquisicion_t Modo, resultado_prueba_t *Promedio)
{
    memset(Promedio, 0, sizeof(*Promedio));
    Promedio->Modo = Modo;
    for (uint32_t i = 0; i < NUMERO_REPETICIONES; i++) {
        if (!Disponibles[Modo][i]) return false;
        const resultado_prueba_t *R = &Resultados[Modo][i];
        Promedio->DuracionSegundos += R->DuracionSegundos;
        Promedio->VoltajePromedio_V += R->VoltajePromedio_V;
        Promedio->CorrientePromedio_mA += R->CorrientePromedio_mA;
        Promedio->PotenciaPromedio_mW += R->PotenciaPromedio_mW;
        Promedio->EnergiaAcumulada_J += R->EnergiaAcumulada_J;
        Promedio->MuestrasValidas += R->MuestrasValidas;
        Promedio->MuestrasPerdidas += R->MuestrasPerdidas;
        Promedio->Errores += R->Errores;
        Promedio->BloquesProcesados += R->BloquesProcesados;
        Promedio->LecturasDmaVacias += R->LecturasDmaVacias;
        Promedio->MedicionesInaValidas += R->MedicionesInaValidas;
        Promedio->MedicionesInaInvalidas += R->MedicionesInaInvalidas;
        Promedio->TiempoProcesamientoBloque_ms += R->TiempoProcesamientoBloque_ms;
        Promedio->DesbordamientosDma += R->DesbordamientosDma;
        Promedio->MediaAdc += R->MediaAdc;
        Promedio->ValorRmsAdc += R->ValorRmsAdc;
    }
    double N = NUMERO_REPETICIONES;
    Promedio->DuracionSegundos /= N;
    Promedio->VoltajePromedio_V /= N;
    Promedio->CorrientePromedio_mA /= N;
    Promedio->PotenciaPromedio_mW /= N;
    Promedio->EnergiaAcumulada_J /= N;
    Promedio->MuestrasValidas /= NUMERO_REPETICIONES;
    Promedio->MuestrasPerdidas /= NUMERO_REPETICIONES;
    Promedio->Errores /= NUMERO_REPETICIONES;
    Promedio->BloquesProcesados /= NUMERO_REPETICIONES;
    Promedio->LecturasDmaVacias /= NUMERO_REPETICIONES;
    Promedio->MedicionesInaValidas /= NUMERO_REPETICIONES;
    Promedio->MedicionesInaInvalidas /= NUMERO_REPETICIONES;
    Promedio->TiempoProcesamientoBloque_ms /= N;
    Promedio->DesbordamientosDma /= NUMERO_REPETICIONES;
    Promedio->MediaAdc /= N;
    Promedio->ValorRmsAdc /= N;
    return true;
}
bool ObtenerComparacionProcesamiento(comparacion_resultados_t *Comparacion)
{
    if (!Comparacion || Estado.Activa) return false;
    memset(Comparacion, 0, sizeof(*Comparacion));
    if (!PromediarResultados(MODO_SIN_DMA, &Comparacion->SinDma) ||
        !PromediarResultados(MODO_CON_DMA, &Comparacion->ConDma)) return false;
    double EBase = Comparacion->SinDma.EnergiaAcumulada_J;
    if (EBase > 0.0) {
        Comparacion->ReduccionEnergeticaPorcentaje =
            100.0 * (EBase - Comparacion->ConDma.EnergiaAcumulada_J) / EBase;
        Comparacion->PorcentajeValido = true;
    }
    return true;
}
