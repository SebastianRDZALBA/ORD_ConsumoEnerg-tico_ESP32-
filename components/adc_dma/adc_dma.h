
#ifndef ADC_DMA_H
#define ADC_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool InicializarAdcDma(void);

bool IniciarAdcDma(void);

bool LeerBloqueAdcDma(uint16_t *Muestras,
                      size_t Capacidad,
                      size_t *CantidadLeida,
                      size_t *CantidadInvalida);

uint32_t ConsultarDesbordamientosAdcDma(void);

bool DetenerAdcDma(void);

bool FinalizarAdcDma(void);

#endif
