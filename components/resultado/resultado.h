
#ifndef RESULTADO_H
#define RESULTADO_H

#include "procesamiento.h"

void MostrarTituloResultado(void);

void MostrarInicializacionResultado(void);

void MostrarEstabilizacionResultado(void);

void MostrarVerificacionSensorResultado(void);

void MostrarReportePeriodicoResultado(const resultado_prueba_t *Resultado);

void MostrarPruebaFinalizadaResultado(void);

void MostrarContinuacionResultado(void);

void MostrarComparacionResultado(const comparacion_resultados_t *Comparacion);

#endif
