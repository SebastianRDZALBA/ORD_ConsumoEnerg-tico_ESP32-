
#ifndef ADC_POT_H
#define ADC_POT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool InicializarAdcPot(void);

bool LeerMuestraAdcPot(uint16_t *Valor);

bool LeerBloqueAdcPot(uint16_t *Muestras,
                       size_t CantidadSolicitada,
                       size_t *CantidadLeida);

bool FinalizarAdcPot(void);

#endif
