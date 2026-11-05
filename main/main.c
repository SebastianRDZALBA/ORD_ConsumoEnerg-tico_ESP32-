// Creado por Sebastián Rodríguez De Alba | 2177777 | MARTES N4N6 | GPO001 | En asesoria del Doc. Hector Guerrero Barron Gonzales
// Examen ordinario - Implementación del DMA
// DESCRIPCIÓN DEL PROYECTO: Compara el consumo energético del ESP32 al leer un potenciómetro mediante ADC con y sin DMA
// Utiliza el INA219 para medir el consumo y muestra los resultados por UART

//Librerías
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "config.h"
#include "adc_pot.h"
#include "adc_dma.h"
#include "sensor_ina.h"
#include "procesamiento.h"
#include "resultado.h"
#include "control.h"

// 1. PARÁMETROS INICIALES
#define MICROSEGUNDOS_POR_MILISEGUNDO 1000LL  // us a ms
#define TIEMPO_ESPERA_MONITOR_MS 15000U       // Da 15 segundos para abrir el monitor UART.

// 2. ESTADOS Y VARIABLES DEL SISTEMA

// Etapas por las que pasa el programa durante el experimento.
typedef enum 
{
    ESTADO_ESTABILIZACION,
    ESTADO_INICIAR_PRUEBA,
    ESTADO_ADQUISICION,
    ESTADO_FINALIZAR_PRUEBA,
    ESTADO_CAMBIAR_PRUEBA,
    ESTADO_COMPARACION,
    ESTADO_FINALIZADO,
    ESTADO_ERROR
} estado_sistema_t;

static estado_sistema_t EstadoSistema = ESTADO_ESTABILIZACION; // Etapa inicial
static modo_adquisicion_t ModoActual = MODO_SIN_DMA;           // Primer método de adquisición
static uint8_t RepeticionActual = 1;                           // Número de repetición
static int64_t InicioEstabilizacionUs = 0;                     // Momento en que comienza la estabilización
static int64_t InicioPausaUs = 0;                              // Momento en que comienza la pausa entre pruebas
static bool SensorInicializado = false;                        // Indica si se inició el INA219
static bool SensorVerificado = false;                          // Indica si se comprobó su comunicación
static uint32_t SegundoEstabilizacion = 0;  // Cuenta las lecturas de estabilización.
static bool ErrorMostrado = false;                             // Evita repetir el mensaje de error
static const char *DescripcionError = "";                      // Guarda el mensaje del error ocurrido

// 3. PROTOTIPOS DE FUNCIONES
static void RegistrarErrorSistema(const char *Mensaje);
static void PrepararSiguientePrueba(void);

// 4. main (inicializa sistema - una repetición de cada método; conserva el switch de estados)
void app_main(void)
{
    // 5. ESPERAR PARA ABRIR EL MONITOR UART
    vTaskDelay(pdMS_TO_TICKS(TIEMPO_ESPERA_MONITOR_MS)); // Espera 15 segundos antes de imprimir (para alcanzar a conectar todo físicamente xd)

    // 6. INICIALIZAR LOS MÓDULOS DEL EXPERIMENTO
    MostrarTituloResultado();                      // Muestra la bienvenida
    MostrarInicializacionResultado();              // Anuncia el inicio del sistema
    InicializarProcesamiento();                    // Prepara dónde guardar los resultados
    SensorInicializado = InicializarSensorIna();   // Configura la comunicación con el INA219 I2C

    // 7. COMPROBAR QUE EL SENSOR SE INICIÓ
    if (!SensorInicializado) // Si no pudo iniciar el INA219
    {
        RegistrarErrorSistema("No se pudo inicializar el INA219");
    }
    else // Si el sensor se inició, comienza la estabilización
    {
        MostrarEstabilizacionResultado();               // Muestra el aviso de estabilización
        InicioEstabilizacionUs = esp_timer_get_time();  // Comienza el conteo de estabilización.
        if (!IniciarEstabilizacionControl()) RegistrarErrorSistema("No se pudo iniciar ADC de estabilización");
    }

    // 8. CICLO PRINCIPAL
    //FLUJO: Estabilizar -> Iniciar prueba -> Adquirir datos -> Finalizar prueba -> Cambiar método -> Repetir -> Comparar resultados -> Finalizar
    while (true)
    {

        // SWITCH DE SELECCIÓN DE LA ETAPA DEL EXPERIMENTO
        switch (EstadoSistema)
        {
            // CASO 1. ESTABILIZACIÓN
            // Espera 10 segundos y verifica la comunicación con el INA219.
            case ESTADO_ESTABILIZACION:
                // Durante la estabilización, leer INA219 y el POT cada segundo.
                if ((esp_timer_get_time() - InicioEstabilizacionUs) / 1000000 >= (int64_t)(SegundoEstabilizacion + 1) && SegundoEstabilizacion < TIEMPO_ESTABILIZACION_MS / 1000U) //Obtiene el tiempo -> calcula cuant tiempo paso desde que comenzó -> convierte a ms -> verifica que no se haya terminado el tiempo de estabilización.
                {
                    SegundoEstabilizacion++;                          //Aumenta el contador 1s
                    LeerEstabilizacionControl(SegundoEstabilizacion); //Lee el INA219 y adc del POT cada segundo durante la estabilización
                }
                // Comprueba si ya pasaron los 10 segundos de estabilización.
                if ((esp_timer_get_time() - InicioEstabilizacionUs) / MICROSEGUNDOS_POR_MILISEGUNDO >= TIEMPO_ESTABILIZACION_MS) // Calcula el tiempo transcurrido desde el inicio y permite continuar cuando hayan pasado 10 segundos
                {
                    // Verifica el sensor una sola vez durante todo el experimento.
                    if (!SensorVerificado)
                    {
                        MostrarVerificacionSensorResultado(); // Anuncia la verificación.
                        // Comprueba que el INA219 responda correctamente.
                        if (!VerificarSensorIna())
                        {
                            RegistrarErrorSistema("El INA219 no responde correctamente"); // Detiene el sistema.
                            break;
                        }
                        SensorVerificado = true; // Marca que el sensor ya fue verificado.
                    }
                    FinalizarEstabilizacionControl();       // Libera ADC One-shot antes de cada prueba.
                    EstadoSistema = ESTADO_INICIAR_PRUEBA; // Pasa a preparar el ADC.
                }
                break;

            // CASO 2. INICIAR PRUEBA
            // Prepara el ADC sin DMA o con DMA, según el método seleccionado.
            case ESTADO_INICIAR_PRUEBA:
                if (!IniciarPruebaControl(ModoActual, RepeticionActual)) // Detiene el sistema si falla
                {
                    RegistrarErrorSistema("No se pudo iniciar la prueba ADC"); 
                }
                else // Si inició correctamente, comienza la adquisición.
                {
                    EstadoSistema = ESTADO_ADQUISICION;
                }
                break;

            // CASO 3. ADQUISICIÓN DE DATOS
            // Cada vuelta toma y procesa un bloque de 500 muestras; después revisa INA219 y UART.
            // Es la secuencia del experimento de referencia, adaptada a nuestros módulos.
            case ESTADO_ADQUISICION:
                if (PruebaTerminadaProcesamiento())
                {
                    EstadoSistema = ESTADO_FINALIZAR_PRUEBA;
                    break;
                }
                // LECTURA DEL POTENCIÓMETRO CON ADC
                // La función espera las 500 muestras sin DMA o lee una trama con DMA.
                // Después del bloque lee el INA219 si pasó 1 s y reporta por UART cada 5 s.
                if (!AdquirirMuestrasControl())
                    RegistrarErrorSistema("Fallo en la adquisición o procesamiento ADC");
                break;

            // CASO 4. FINALIZAR PRUEBA ( Muestra el último reporte y guarda los resultados de los 120 segundos)
            case ESTADO_FINALIZAR_PRUEBA:
                if (!FinalizarPruebaControl()) // Intenta detener el ADC y guardar los resultados obtenidos.
                {
                    RegistrarErrorSistema("No fue posible finalizar o guardar la prueba"); // Informa fallo
                }
                else // Si finalizó correctamente, comienza la pausa.
                {
                    MostrarResumenFinalControl(ModoActual, RepeticionActual); // Muestra el resumen después de registrar la última lectura.
                    MostrarPruebaFinalizadaResultado();     // Muestra el mensaje de finalización.
                    InicioPausaUs = esp_timer_get_time();   // Guarda la hora para contar la pausa.
                    EstadoSistema = ESTADO_CAMBIAR_PRUEBA;  // Pasa a preparar la siguiente prueba.
                }
                break;

            // CASO 5. CAMBIAR DE PRUEBA (Espera una pausa y cambia entre los métodos sin DMA y con DMA)
            case ESTADO_CAMBIAR_PRUEBA:
                if ((esp_timer_get_time() - InicioPausaUs) / MICROSEGUNDOS_POR_MILISEGUNDO >= PAUSA_ENTRE_PRUEBAS_MS)// Comprueba si ya terminó la pausa entre pruebas.
                {
                    VerificarAlimentacionControl(); // Comprueba V/I antes de la siguiente prueba.
                    PrepararSiguientePrueba(); // Selecciona el siguiente método o repetición.
                }
                break;

            // CASO 6. COMPARACIÓN DE RESULTADOS (Compara las repeticiones realmente configuradas)
            case ESTADO_COMPARACION:
            {
                comparacion_resultados_t Comparacion; // Guarda los datos que aparecerán en la tabla.

                if (ObtenerComparacionProcesamiento(&Comparacion))// Comprueba si se pudo calcular la comparación final.
                {
                    MostrarComparacionResultado(&Comparacion); // Muestra la tabla sin DMA vs. con DMA.
                    EstadoSistema = ESTADO_FINALIZADO; // Marca que terminó el experimento.
                }
                else // Si no se pudo calcular la comparación, entra al estado de error.
                {
                    RegistrarErrorSistema("No se pudo calcular la comparacion final");
                }

                // Comprueba si el INA219 sigue inicializado.
                if (SensorInicializado)
                {
                    FinalizarSensorIna(); // Libera la comunicación con el INA219.
                    SensorInicializado = false; // Marca que el sensor ya se cerró.
                }
                break;
            }

            // CASO 7. EXPERIMENTO FINALIZADO (Ya no realiza mediciones ni vuelve a mostrar la tabla)
            case ESTADO_FINALIZADO:
                break;

            // CASO 8. ERROR DEL SISTEMA (Muestra el fallo y libera los componentes utilizados)
            case ESTADO_ERROR:
                if (!ErrorMostrado)// Comprueba que el error no se haya mostrado anteriormente.
                {
                    printf("\nERROR DEL SISTEMA: %s\n", DescripcionError);
                    LiberarAdquisicionControl(); // Detiene y libera el ADC que se estaba utilizando.
                    
                    if (SensorInicializado) // Comprueba si el INA219 sigue inicializado.
                    {
                        FinalizarSensorIna();        // Libera la comunicación con el sensor.
                        SensorInicializado = false;  // Marca que el sensor ya se cerró.
                    }
                    ErrorMostrado = true; // Evita mostrar y atender el mismo error varias veces.
                }
                break;
        }
        // FIN DEL SWITCH DE ESTADOS

        // 9. DAR TIEMPO A OTRAS TAREAS (Solo fuera de la adquisición, como el ciclo de referencia)
        if (EstadoSistema != ESTADO_ADQUISICION)
        {
            TickType_t Espera = pdMS_TO_TICKS(RETARDO_CICLO_PRINCIPAL_MS); // Convierte la espera a ticks de FreeRTOS.
            vTaskDelay(Espera > 0 ? Espera : 1);                           // Cede al menos un tick para ejecutar otras tareas.
        }
    }
}

// 10. FUNCIONES AUXILIARES

// 11. REGISTRAR ERROR DEL SISTEMA (Guarda el mensaje del fallo y cambia al estado de error)
static void RegistrarErrorSistema(const char *Mensaje)
{
    DescripcionError = Mensaje; // Guarda el texto que explica el problema.
    EstadoSistema = ESTADO_ERROR; // Indica al switch que debe atender el error.
}

// 12. PREPARAR LA SIGUIENTE PRUEBA (alterna entre sin DMA y con DMA hasta completar las repeticiones)
static void PrepararSiguientePrueba(void)
{
    // Selecciona el siguiente método de adquisición.
    if (ModoActual == MODO_SIN_DMA)
    {
        ModoActual = MODO_CON_DMA; // Después de sin DMA, sigue con DMA.
    }
    else // Si terminó con DMA, comienza una nueva repetición sin DMA.
    {
        ModoActual = MODO_SIN_DMA; // Prepara el método sin DMA.
        RepeticionActual++; // Aumenta el número de repetición.
    }

    // Comprueba si ya se completaron las repeticiones configuradas de ambos métodos.
    if (RepeticionActual > NUMERO_REPETICIONES)
    {
        EstadoSistema = ESTADO_COMPARACION; // Pasa a mostrar la tabla final.
    }
    else // Si todavía faltan pruebas, prepara otra estabilización.
    {
        MostrarContinuacionResultado(); // Avisa que comenzará la siguiente prueba.
        SegundoEstabilizacion = 0;
        if (!IniciarEstabilizacionControl()) {
            RegistrarErrorSistema("No se pudo reiniciar ADC de estabilización");
        } else {
            InicioEstabilizacionUs = esp_timer_get_time(); // Reinicia el conteo de estabilización.
            EstadoSistema = ESTADO_ESTABILIZACION; // Espera 10 segundos antes de medir otra vez.
        }
    }
}