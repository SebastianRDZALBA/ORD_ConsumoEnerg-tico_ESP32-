
#ifndef CONFIG_ENERGETICA_H
#define CONFIG_ENERGETICA_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

#define PIN_POTENCIOMETRO                  GPIO_NUM_34
#define PIN_SDA_INA219                     GPIO_NUM_21
#define PIN_SCL_INA219                     GPIO_NUM_22
#define PIN_TX_UART                        GPIO_NUM_1

#define UNIDAD_ADC                         ADC_UNIT_1
#define CANAL_ADC_POTENCIOMETRO            ADC_CHANNEL_6
#define ATENUACION_ADC                     ADC_ATTEN_DB_12
#define RESOLUCION_ADC                     ADC_BITWIDTH_12
#define VALOR_MAXIMO_ADC                   4095U

// Frecuencia EFECTIVA para ambos metodos (10 kHz, como en la prueba de referencia).
#define FRECUENCIA_MUESTREO_HZ             10000U
// El ADC continuo conserva una frecuencia fisica de 20 kHz; se utiliza una de cada dos lecturas.
#define FRECUENCIA_CONVERSION_DMA_HZ       20000U
#define FACTOR_DECIMACION_DMA              (FRECUENCIA_CONVERSION_DMA_HZ / FRECUENCIA_MUESTREO_HZ)
// Sondeo sin DMA: una lectura cada 100 us, con espera activa.
// Se registra el retraso mediante el tiempo y el conteo final de muestras.
#define PERIODO_MUESTREO_US                (1000000U / FRECUENCIA_MUESTREO_HZ)
#define MUESTRAS_POR_BLOQUE                500U // 50 ms de muestras utiles a 10 kHz.

#define MODO_CONVERSION_DMA                ADC_CONV_SINGLE_UNIT_1
#define FORMATO_SALIDA_DMA                 ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define TAMANO_TRAMA_DMA_BYTES             (MUESTRAS_POR_BLOQUE * FACTOR_DECIMACION_DMA * SOC_ADC_DIGI_RESULT_BYTES)
#define TAMANO_BUFFER_DMA_BYTES            (TAMANO_TRAMA_DMA_BYTES * 4U)
#define TIEMPO_ESPERA_DMA_MS               100U

#define PUERTO_I2C                         0
#define FRECUENCIA_I2C_HZ                  100000U
#define DIRECCION_INA219                   0x40
#define TIEMPO_ESPERA_I2C_MS               100U

#define RESISTENCIA_SHUNT_OHMIOS           0.1f
#define CALIBRACION_INA219                 4096U
#define PASO_CORRIENTE_INA219_MA           0.1f
#define PASO_POTENCIA_INA219_MW            2.0f
#define PASO_VOLTAJE_BUS_INA219_MV         4.0f
#define PASO_VOLTAJE_SHUNT_INA219_MV       0.01f
#define CORRIENTE_MAXIMA_INA219_MA         3200.0f

#define TIEMPO_ESTABILIZACION_MS           10000U
#define DURACION_PRUEBA_MS                 120000U
#define PAUSA_ENTRE_PRUEBAS_MS             2000U
#define PERIODO_LECTURA_INA219_MS          1000U
#define PERIODO_REPORTE_UART_MS            5000U
#define NUMERO_REPETICIONES                1U
#define RETARDO_CICLO_PRINCIPAL_MS         1U

#define VELOCIDAD_UART_BAUDIOS             115200U

#endif