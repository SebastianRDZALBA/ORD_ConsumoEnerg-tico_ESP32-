#ifndef PROCESAMIENTO_H
#define PROCESAMIENTO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sensor_ina.h"

typedef enum { MODO_SIN_DMA = 0, MODO_CON_DMA = 1 } modo_adquisicion_t;

typedef struct {
    modo_adquisicion_t Modo;
    uint8_t Repeticion;
    double DuracionSegundos, VoltajePromedio_V, CorrientePromedio_mA;
    double PotenciaPromedio_mW, EnergiaAcumulada_J;
    uint64_t MuestrasValidas, MuestrasPerdidas;
    uint32_t Errores, DesbordamientosDma;
    uint32_t BloquesProcesados, LecturasDmaVacias;
    uint32_t MedicionesInaValidas, MedicionesInaInvalidas;
    double MediaAdc, ValorRmsAdc;
    uint16_t MinimoAdc, MaximoAdc;
    double FrecuenciaEfectiva_Hz, TiempoProcesamientoBloque_ms;
} resultado_prueba_t;

typedef struct {
    resultado_prueba_t SinDma, ConDma;
    double ReduccionEnergeticaPorcentaje;
    bool PorcentajeValido;
} comparacion_resultados_t;

void InicializarProcesamiento(void);
bool IniciarPruebaProcesamiento(modo_adquisicion_t Modo, uint8_t Repeticion);
bool ProcesarBloqueProcesamiento(const uint16_t *Muestras, size_t Cantidad);
bool RegistrarMedicionProcesamiento(const medicion_ina_t *Medicion);
void RegistrarMedicionInvalidaInaProcesamiento(void);
void RegistrarLecturaDmaVaciaProcesamiento(void);
void RegistrarMuestrasPerdidasProcesamiento(uint32_t Cantidad);
void RegistrarErrorProcesamiento(void);
void ActualizarDesbordamientosProcesamiento(uint32_t Cantidad);
uint32_t ObtenerTiempoTranscurridoProcesamiento(void);
bool PruebaTerminadaProcesamiento(void);
bool ObtenerResumenProcesamiento(resultado_prueba_t *Resultado);
bool FinalizarPruebaProcesamiento(void);
bool ConsultarResultadoProcesamiento(modo_adquisicion_t Modo, uint8_t Repeticion, resultado_prueba_t *Resultado);
bool ObtenerComparacionProcesamiento(comparacion_resultados_t *Comparacion);
#endif
