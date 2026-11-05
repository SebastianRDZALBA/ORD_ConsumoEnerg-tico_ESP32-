#ifndef CONTROL_H
#define CONTROL_H
#include <stdbool.h>
#include <stdint.h>
#include "procesamiento.h"

bool IniciarEstabilizacionControl(void);
void LeerEstabilizacionControl(uint32_t Segundo);
void FinalizarEstabilizacionControl(void);
void VerificarAlimentacionControl(void);
bool IniciarPruebaControl(modo_adquisicion_t Modo, uint8_t Repeticion);
bool AdquirirMuestrasControl(void);
void RegistrarConsumoControl(void);
void MostrarReporteControl(void);
void MostrarResumenFinalControl(modo_adquisicion_t Modo, uint8_t Repeticion);
bool FinalizarPruebaControl(void);
void LiberarAdquisicionControl(void);
#endif
