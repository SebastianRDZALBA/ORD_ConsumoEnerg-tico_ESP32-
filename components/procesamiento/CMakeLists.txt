idf_component_register(
    SRCS "procesamiento.c"
    INCLUDE_DIRS "."
    REQUIRES sensor_ina
    PRIV_REQUIRES config esp_timer
)