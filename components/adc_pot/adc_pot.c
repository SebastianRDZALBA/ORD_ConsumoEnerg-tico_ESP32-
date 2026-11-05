
#include "adc_pot.h"
#include "config.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t ManejadorAdcPot = NULL;

bool InicializarAdcPot(void)
{
    if (ManejadorAdcPot != NULL)
    {
        return true;
    }

    adc_oneshot_unit_init_cfg_t ConfiguracionUnidad = {
        .unit_id = UNIDAD_ADC,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    if (adc_oneshot_new_unit(&ConfiguracionUnidad,
                             &ManejadorAdcPot) != ESP_OK)
    {
        ManejadorAdcPot = NULL;
        return false;
    }

    adc_oneshot_chan_cfg_t ConfiguracionCanal = {
        .atten = ATENUACION_ADC,
        .bitwidth = RESOLUCION_ADC
    };

    if (adc_oneshot_config_channel(ManejadorAdcPot,
                                   CANAL_ADC_POTENCIOMETRO,
                                   &ConfiguracionCanal) != ESP_OK)
    {
        adc_oneshot_del_unit(ManejadorAdcPot);
        ManejadorAdcPot = NULL;
        return false;
    }

    return true;
}

bool LeerMuestraAdcPot(uint16_t *Valor)
{
    if (ManejadorAdcPot == NULL || Valor == NULL)
    {
        return false;
    }

    int Lectura = 0;

    if (adc_oneshot_read(ManejadorAdcPot,
                         CANAL_ADC_POTENCIOMETRO,
                         &Lectura) != ESP_OK)
    {
        return false;
    }

    if (Lectura < 0 || Lectura > VALOR_MAXIMO_ADC)
    {
        return false;
    }

    *Valor = (uint16_t)Lectura;

    return true;
}

bool LeerBloqueAdcPot(uint16_t *Muestras,
                       size_t CantidadSolicitada,
                       size_t *CantidadLeida)
{
    if (CantidadLeida == NULL)
    {
        return false;
    }

    *CantidadLeida = 0;

    if (ManejadorAdcPot == NULL ||
        Muestras == NULL ||
        CantidadSolicitada == 0)
    {
        return false;
    }

    for (size_t Indice = 0; Indice < CantidadSolicitada; Indice++)
    {
        if (!LeerMuestraAdcPot(&Muestras[Indice]))
        {
            return false;
        }

        (*CantidadLeida)++;
    }

    return true;
}

bool FinalizarAdcPot(void)
{
    if (ManejadorAdcPot == NULL)
    {
        return true;
    }

    if (adc_oneshot_del_unit(ManejadorAdcPot) != ESP_OK)
    {
        return false;
    }

    ManejadorAdcPot = NULL;

    return true;
}
