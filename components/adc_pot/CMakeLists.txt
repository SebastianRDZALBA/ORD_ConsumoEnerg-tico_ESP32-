
idf_component_register(
    SRCS "adc_pot.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES config esp_adc
)