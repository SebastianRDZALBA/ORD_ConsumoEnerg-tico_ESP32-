#include "adc_dma.h"
#include "config.h"

#include "esp_adc/adc_continuous.h"
#include "esp_attr.h"
#include "esp_err.h"

static adc_continuous_handle_t ManejadorAdcDma = NULL;
static bool AdquisicionActiva = false;
static volatile uint32_t ContadorDesbordamientos = 0;
static uint32_t ConversionesCrudas = 0;

// Trama de 1000 conversiones a 20 kHz: se conservan 500 muestras a 10 kHz.
static uint8_t DatosCrudos[TAMANO_TRAMA_DMA_BYTES];
static adc_continuous_data_t DatosConvertidos[MUESTRAS_POR_BLOQUE * FACTOR_DECIMACION_DMA];

static bool IRAM_ATTR RegistrarDesbordamiento(adc_continuous_handle_t Manejador,
                                              const adc_continuous_evt_data_t *Evento,
                                              void *Contexto)
{
    (void)Manejador;
    (void)Evento;
    (void)Contexto;
    ContadorDesbordamientos++;
    return false;
}

bool InicializarAdcDma(void)
{
    if (ManejadorAdcDma != NULL) return true;

    adc_continuous_handle_cfg_t ConfiguracionManejador = {
        .max_store_buf_size = TAMANO_BUFFER_DMA_BYTES,
        .conv_frame_size = TAMANO_TRAMA_DMA_BYTES,
        .flags.flush_pool = 0
    };
    if (adc_continuous_new_handle(&ConfiguracionManejador, &ManejadorAdcDma) != ESP_OK) {
        ManejadorAdcDma = NULL;
        return false;
    }

    adc_digi_pattern_config_t PatronAdc = {
        .atten = ATENUACION_ADC,
        .channel = CANAL_ADC_POTENCIOMETRO,
        .unit = UNIDAD_ADC,
        .bit_width = RESOLUCION_ADC
    };
    adc_continuous_config_t ConfiguracionAdc = {
        .pattern_num = 1,
        .adc_pattern = &PatronAdc,
        .sample_freq_hz = FRECUENCIA_CONVERSION_DMA_HZ,
        .conv_mode = MODO_CONVERSION_DMA,
        .format = FORMATO_SALIDA_DMA
    };
    if (adc_continuous_config(ManejadorAdcDma, &ConfiguracionAdc) != ESP_OK) {
        adc_continuous_deinit(ManejadorAdcDma);
        ManejadorAdcDma = NULL;
        return false;
    }

    adc_continuous_evt_cbs_t EventosAdc = {.on_pool_ovf = RegistrarDesbordamiento};
    if (adc_continuous_register_event_callbacks(ManejadorAdcDma, &EventosAdc, NULL) != ESP_OK) {
        adc_continuous_deinit(ManejadorAdcDma);
        ManejadorAdcDma = NULL;
        return false;
    }

    ContadorDesbordamientos = 0;
    ConversionesCrudas = 0;
    return true;
}

bool IniciarAdcDma(void)
{
    if (ManejadorAdcDma == NULL) return false;
    if (AdquisicionActiva) return true;
    if (adc_continuous_flush_pool(ManejadorAdcDma) != ESP_OK) return false;
    ContadorDesbordamientos = 0;
    ConversionesCrudas = 0;
    if (adc_continuous_start(ManejadorAdcDma) != ESP_OK) return false;
    AdquisicionActiva = true;
    return true;
}

bool LeerBloqueAdcDma(uint16_t *Muestras, size_t Capacidad,
                      size_t *CantidadLeida, size_t *CantidadInvalida)
{
    if (!CantidadLeida || !CantidadInvalida) return false;
    *CantidadLeida = 0;
    *CantidadInvalida = 0;
    if (!ManejadorAdcDma || !AdquisicionActiva || !Muestras ||
        Capacidad < MUESTRAS_POR_BLOQUE) return false;

    // Igual que la referencia: reunir la trama completa, incluso en varias lecturas.
    uint32_t BytesTotales = 0;
    uint32_t BytesParciales = 1;
    while (BytesTotales < TAMANO_TRAMA_DMA_BYTES && BytesParciales > 0) {
        BytesParciales = 0;
        esp_err_t Estado = adc_continuous_read(
            ManejadorAdcDma,
            DatosCrudos + BytesTotales,
            TAMANO_TRAMA_DMA_BYTES - BytesTotales,
            &BytesParciales,
            TIEMPO_ESPERA_DMA_MS);
        if (Estado != ESP_OK && Estado != ESP_ERR_TIMEOUT) return false;
        BytesTotales += BytesParciales;
    }

    if (BytesTotales == 0) return true;
    if (BytesTotales % SOC_ADC_DIGI_RESULT_BYTES != 0) return false;

    uint32_t CantidadConvertida = 0;
    if (adc_continuous_parse_data(ManejadorAdcDma, DatosCrudos,
                                  BytesTotales, DatosConvertidos,
                                  &CantidadConvertida) != ESP_OK) return false;

    for (uint32_t Indice = 0; Indice < CantidadConvertida; Indice++) {
        bool Conservar = ConversionesCrudas % FACTOR_DECIMACION_DMA == 0;
        ConversionesCrudas++;
        if (!Conservar) continue;
        if (!DatosConvertidos[Indice].valid ||
            DatosConvertidos[Indice].unit != UNIDAD_ADC ||
            DatosConvertidos[Indice].channel != CANAL_ADC_POTENCIOMETRO ||
            DatosConvertidos[Indice].raw_data > VALOR_MAXIMO_ADC) {
            (*CantidadInvalida)++;
            continue;
        }
        if (*CantidadLeida >= Capacidad) return false;
        Muestras[(*CantidadLeida)++] = (uint16_t)DatosConvertidos[Indice].raw_data;
    }
    return true;
}

uint32_t ConsultarDesbordamientosAdcDma(void) { return ContadorDesbordamientos; }

bool DetenerAdcDma(void)
{
    if (ManejadorAdcDma == NULL) return false;
    if (!AdquisicionActiva) return true;
    if (adc_continuous_stop(ManejadorAdcDma) != ESP_OK) return false;
    AdquisicionActiva = false;
    return true;
}

bool FinalizarAdcDma(void)
{
    if (ManejadorAdcDma == NULL) return true;
    if (!DetenerAdcDma()) return false;
    if (adc_continuous_deinit(ManejadorAdcDma) != ESP_OK) return false;
    ManejadorAdcDma = NULL;
    AdquisicionActiva = false;
    return true;
}
