idf_component_register(
    SRCS "sensor_ina.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES config esp_driver_i2c
)