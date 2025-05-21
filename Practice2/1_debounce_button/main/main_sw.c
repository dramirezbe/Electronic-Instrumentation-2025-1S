#include "tim_ch_duty.h" // Asume que este archivo proporciona definiciones necesarias
#include "io_utils.h"    // Asume que este archivo proporciona isr_io_config y is_debounced

#include <stdio.h>
#include "esp_err.h"     // Se mantiene, común en proyectos ESP-IDF

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // Necesario para xTaskGetTickCount

#include "driver/gpio.h"
#include "esp_log.h"     // Se mantiene, común para logging en ESP-IDF

#define BTN_GPIO            GPIO_NUM_25   // Botón conectado al GPIO 25
#define DEBOUNCE_TIME_MS    20            // Tiempo de debounce en milisegundos

// Bandera de interrupción: IRAM para disponer de código en memoria rápida
// y LEVEL3 para asignar una alta prioridad (nivel 3 es un buen compromiso).
#define ISR_FLAGS           (ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3)

// Contador global de interrupciones (declarado como volatile ya que se modifica desde la ISR)
volatile int interrupt_count = 0;

// Variables para el reinicio del contador por inactividad
// last_interrupt_count: Almacena el valor del contador en la última verificación
static int last_interrupt_count = 0;
// last_change_time: Almacena el tiempo (en ticks) de la última vez que interrupt_count cambió
static TickType_t last_change_time;

// Variable estática para almacenar el tiempo del último evento para el debounce
static TickType_t last_button_event_time = 0;

// Manejador de interrupción: se ejecuta en cada flanco descendente del botón
// IRAM_ATTR asegura que esta función se coloque en la RAM rápida para una ejecución eficiente de la ISR.
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // Obtener el tiempo actual dentro de la ISR
    TickType_t current_time_isr = xTaskGetTickCountFromISR();

    // Comprobar si ha pasado el tiempo de debounce desde el último evento
    if (is_debounced(current_time_isr, last_button_event_time, DEBOUNCE_TIME_MS)) {
        // Solo se incrementa el contador si el botón está debounced
        interrupt_count++;
        // Actualizar el tiempo del último evento
        last_button_event_time = current_time_isr;
    }
}

void app_main(void) {

    // Configuración del GPIO para el botón:
    // BTN_GPIO: Número del pin GPIO
    // true: Configurado como entrada
    // true: Habilitar resistencia pull-up interna (útil para botones que conectan a tierra)
    // false: No usar modo open-drain
    // GPIO_INTR_NEGEDGE: Interrumpir en el flanco descendente (cuando el botón es presionado si usa pull-up)
    isr_io_config(BTN_GPIO, true, true, false, GPIO_INTR_NEGEDGE);

    // Instalar el servicio de interrupciones del GPIO
    // ISR_FLAGS: Banderas de configuración para el servicio de interrupciones
    gpio_install_isr_service(ISR_FLAGS);
    // Añadir el manejador de interrupción al pin del botón
    // BTN_GPIO: Pin GPIO al que se asocia la interrupción
    // gpio_isr_handler: Función que se ejecutará cuando ocurra la interrupción
    // (void*) BTN_GPIO: Argumento que se pasa al manejador (en este caso, el propio número de GPIO)
    gpio_isr_handler_add(BTN_GPIO, gpio_isr_handler, (void*) BTN_GPIO);

    printf("-------------Ready---------------\n");

    // Inicializar el tiempo de último cambio al inicio de la aplicación
    // Esto asegura que el contador no se reinicie inmediatamente al inicio.
    last_change_time = xTaskGetTickCount();
    // Asegurarse de que el valor inicial de last_interrupt_count coincida con interrupt_count
    last_interrupt_count = 0;
    // Inicializar last_button_event_time al inicio
    last_button_event_time = xTaskGetTickCount();


    // Bucle principal: se ejecuta continuamente.
    // Cada 50ms se verifica el contador y se imprime su valor.
    // Si el contador no cambia en 2 segundos, se reinicia a cero.
    while (1) {
        // Obtener el tiempo actual en ticks del FreeRTOS
        TickType_t current_time = xTaskGetTickCount();

        // Comprobar si el valor del contador de interrupciones ha cambiado
        if (interrupt_count != last_interrupt_count) {
            // Si ha cambiado, actualizar last_interrupt_count y el tiempo del último cambio
            last_interrupt_count = interrupt_count;
            last_change_time = current_time;
        } else {
            // Si el contador no ha cambiado, verificar si han pasado 2 segundos (2000 ms)
            // Multiplicar por portTICK_PERIOD_MS para convertir ticks a milisegundos
            if ((current_time - last_change_time) * portTICK_PERIOD_MS >= 2000) {
                // Si han pasado 2 segundos sin cambios, reiniciar el contador
                // y también actualizar last_interrupt_count y last_change_time
                // para que el reinicio no se repita en el siguiente ciclo si no hay interrupciones.
                interrupt_count = 0;
                last_interrupt_count = 0;
                last_change_time = current_time; // Reiniciar el temporizador de inactividad
            }
        }

        // Imprimir el número actual de interrupciones
        printf("Interrupt num: %d\n", interrupt_count);
        // Retardo para no saturar la CPU y permitir que otras tareas del FreeRTOS se ejecuten.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}