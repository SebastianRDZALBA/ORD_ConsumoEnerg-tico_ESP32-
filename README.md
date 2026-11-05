# ORD_ConsumoEnerg-tico_ESP32-
Evaluación del consumo energético del ESP32 mediante adquisición ADC con y sin DMA
# Evaluación energética del ESP32: ADC con y sin DMA

**Proyecto académico desarrollado en C con ESP-IDF 6.1.0 para el ESP32-WROOM-32.**

## Descripción del proyecto

Este proyecto tiene como objetivo evaluar y comparar el consumo energético del microcontrolador ESP32 durante la adquisición de una señal analógica utilizando dos métodos: ADC One-shot sin DMA y ADC continuo asistido por DMA (*Direct Memory Access*).

Para realizar la comparación se utiliza un potenciómetro como señal analógica de entrada y un sensor INA219 para medir el voltaje, la corriente y la potencia consumida por el sistema.

El firmware realiza las pruebas bajo condiciones de muestreo equivalentes, procesa los datos adquiridos y calcula la energía acumulada durante cada experimento. Los resultados se presentan mediante una interfaz UART, incluyendo una tabla comparativa final.

## Objetivo

Analizar las diferencias de consumo energético, adquisición y procesamiento de datos entre los métodos ADC sin DMA y ADC con DMA, considerando la intervención del procesador y la eficiencia de transferencia de muestras.

## Hardware utilizado

- Microcontrolador ESP32-WROOM-32.
- Sensor de corriente, voltaje y potencia INA219.
- Potenciómetro para generar una señal analógica ajustable.
- Adaptador USB-UART CP2102 para el monitor serial.
- Fuente externa regulada de 5 V.
- Protoboard y cables de conexión.

## Conexiones del sistema

| Componente | Conexión ESP32 | Función |
|---|---|---|
| Potenciómetro | GPIO34 (ADC1_CH6) | Entrada analógica |
| INA219 SDA | GPIO21 | Datos I²C |
| INA219 SCL | GPIO22 | Reloj I²C |
| INA219 VCC | 3.3 V | Alimentación del sensor |
| INA219 GND | GND | Tierra común |
| CP2102 RX | GPIO1 (TX0) | Recepción de datos UART |
| CP2102 GND | GND | Tierra común |
| INA219 VIN+ | Positivo de la fuente externa | Entrada de alimentación medida |
| INA219 VIN− | VIN de la placa ESP32 | Salida de alimentación medida |

El potenciómetro se alimenta con 3.3 V y comparte la referencia GND con el microcontrolador.

Durante la medición energética, la placa ESP32 se alimenta desde la fuente externa a través del INA219. El adaptador USB-UART se utiliza únicamente para recibir los mensajes del sistema; sus pines de alimentación permanecen desconectados del ESP32.

## Funcionamiento del firmware

El programa está organizado mediante una máquina de estados que controla las distintas etapas del experimento.

**1. Inicialización**

Se configuran los periféricos, la comunicación I²C y las estructuras utilizadas para almacenar las mediciones y los resultados.

**2. Estabilización**

Durante 10 segundos se comprueba el funcionamiento del INA219 y del ADC antes de comenzar las pruebas.

**3. Adquisición sin DMA**

El ADC utiliza el modo One-shot. El procesador controla cada conversión mediante sondeo activo, con un período objetivo de 100 microsegundos entre muestras.

**4. Adquisición con DMA**

El ADC trabaja en modo continuo a 20 kHz. DMA transfiere los datos a memoria y el programa conserva una de cada dos conversiones para obtener una frecuencia efectiva objetivo de 10 kHz.

**5. Procesamiento**

Las muestras se organizan en bloques de 500 datos. Para cada bloque se calculan estadísticas como promedio, RMS, mínimo y máximo.

**6. Medición energética**

El INA219 registra periódicamente el voltaje y la corriente. A partir de estas mediciones se calcula la potencia y se acumula la energía consumida durante el experimento.

**7. Reporte UART**

Cada cinco segundos se muestra un resumen de las mediciones. Al finalizar ambas pruebas se imprime una tabla comparativa y el porcentaje de reducción energética.

## Parámetros experimentales

| Parámetro | Configuración |
|---|---|
| Frecuencia efectiva objetivo | 10 kHz |
| Frecuencia física ADC con DMA | 20 kHz |
| Resolución ADC | 12 bits |
| Atenuación ADC | 12 dB |
| Muestras por bloque | 500 |
| Duración por prueba | 120 segundos |
| Estabilización inicial | 10 segundos |
| Pausa entre pruebas | 2 segundos |
| Lectura INA219 | Cada segundo |
| Reporte UART | Cada 5 segundos |
| Repeticiones | 1 por método |
| Velocidad UART | 115200 baudios |

## Organización del proyecto

El código está distribuido en módulos para separar las responsabilidades de adquisición, procesamiento, instrumentación y presentación de resultados.

| Archivo | Responsabilidad |
|---|---|
| `main/main.c` | Máquina de estados y secuencia principal del experimento |
| `main/control.c` | Coordinación de pruebas y adquisición de datos |
| `components/config/config.h` | Pines y parámetros generales |
| `components/adc_pot/adc_pot.c` | Adquisición ADC sin DMA |
| `components/adc_dma/adc_dma.c` | Adquisición ADC continua con DMA |
| `components/sensor_ina/sensor_ina.c` | Comunicación y mediciones del INA219 |
| `components/procesamiento/procesamiento.c` | Estadísticas, energía y registro de resultados |
| `components/resultado/resultado.c` | Reportes periódicos y comparación por UART |

Cada módulo dispone de sus archivos de encabezado y configuración CMake correspondientes. El proyecto también incluye el archivo principal `CMakeLists.txt` y la configuración de ESP-IDF.

## Compilación y ejecución

**Requisitos:** ESP-IDF 6.1.0, compilador para ESP32, Python y herramientas de compilación de ESP-IDF correctamente configuradas.

Desde una terminal de ESP-IDF, ubicada en la carpeta principal del proyecto:

**1. Seleccionar el microcontrolador:**

`idf.py set-target esp32`

**2. Configurar el proyecto:**

`idf.py reconfigure`

**3. Compilar el firmware:**

`idf.py build`

**4. Programar el ESP32:**

`idf.py -p COMx flash`

Sustituir `COMx` por el puerto de programación correspondiente.

Para visualizar las mediciones, utilizar el monitor UART a 115200 baudios. Si se utiliza un CP2102 externo, debe seleccionarse su puerto COM para recibir los mensajes.

La configuración de adquisición y del sistema se encuentra en `components/config/config.h`, `sdkconfig` y `sdkconfig.defaults`. Si se regenera la configuración mediante `set-target`, es necesario comprobar que los parámetros experimentales se hayan conservado.

## Resultados experimentales

En una ejecución de 120 segundos por método se obtuvieron los siguientes resultados:

| Métrica | Sin DMA | Con DMA |
|---|---:|---:|
| Voltaje promedio | 5.165 V | 5.172 V |
| Corriente promedio | 42.56 mA | 34.06 mA |
| Potencia promedio | 219.81 mW | 176.17 mW |
| Energía acumulada | 26.3849 J | 21.1294 J |
| Muestras válidas | 1,190,500 | 1,200,000 |
| Muestras perdidas | 9,888 | 254 |
| Errores registrados | 0 | 0 |
| Desbordamientos DMA | No aplica | 0 |

**Reducción energética observada: 19.92 %.**

El método con DMA presentó un menor consumo energético y una reducción en la cantidad de muestras perdidas durante esta ejecución.

Estos valores corresponden a una prueba experimental específica y pueden variar según las condiciones de alimentación, la configuración del microcontrolador y las tareas ejecutadas.

## Consideraciones experimentales

El experimento compara dos estrategias de adquisición, no únicamente la activación o desactivación de DMA.

Sin DMA se utiliza sondeo activo, mientras que con DMA la transferencia de conversiones a memoria se realiza mediante el controlador de adquisición continua.

Aunque ambos métodos mantienen una frecuencia efectiva objetivo equivalente y el mismo procedimiento de procesamiento, su comportamiento interno y la actividad de CPU son diferentes.

La prueba se realizó una vez por método, por lo que los resultados no representan una caracterización estadística de múltiples repeticiones.

## Documentación técnica

- [ESP-IDF Programming Guide v6.1](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/index.html)
- [ADC One-shot](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_oneshot.html)
- [ADC continuo con DMA](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_continuous.html)
- [ESP32-WROOM-32 Datasheet](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
- [INA219 Datasheet](https://www.ti.com/document-viewer/INA219/datasheet)
- [CP2102/9 Datasheet](https://www.silabs.com/documents/public/data-sheets/CP2102-9.pdf)

## Conclusión

El proyecto permitió comparar experimentalmente dos estrategias de adquisición ADC del ESP32 mediante mediciones reales de corriente, voltaje, potencia y energía.

En la ejecución documentada, la adquisición continua con DMA presentó una reducción energética del 19.92 % frente al método de sondeo activo sin DMA, además de registrar menos pérdidas de muestras.

El experimento demuestra la importancia de considerar tanto la adquisición de datos como el trabajo del procesador al evaluar la eficiencia energética de un sistema embebido.
