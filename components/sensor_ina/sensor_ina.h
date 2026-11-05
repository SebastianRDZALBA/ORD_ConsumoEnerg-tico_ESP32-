
#ifndef SENSOR_INA_H
#define SENSOR_INA_H

#include <stdbool.h>

typedef struct
{
    float Voltaje_V;
    float Corriente_mA;
    float Potencia_mW;
} medicion_ina_t;

bool InicializarSensorIna(void);

bool VerificarSensorIna(void);

bool LeerMedicionSensorIna(medicion_ina_t *Medicion);

bool FinalizarSensorIna(void);

#endif
