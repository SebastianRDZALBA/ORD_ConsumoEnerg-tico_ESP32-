idf_component_register(
    SRCS "main.c" "control.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES config adc_pot adc_dma sensor_ina procesamiento resultado esp_timer freertos
)