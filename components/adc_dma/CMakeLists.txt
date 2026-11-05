idf_component_register(
    SRCS "adc_dma.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES config esp_adc
)