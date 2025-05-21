#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "tim_ch_duty.h"
#include "io_utils.h"

// Global variables modified in ISR (declared volatile)
volatile uint32_t pwm_counter = 0;
volatile uint32_t seconds = 0;

// GPIO definitions (adjust according to your wiring)
// PWM_GPIO outputs the PWM signal; INTERRUPT_GPIO receives that signal.
#define PWM_GPIO         GPIO_NUM_25
#define INTERRUPT_GPIO   GPIO_NUM_35

// ISR handler for falling edge pulse detection.
// Called in interrupt context (keep it short and minimal!).
void IRAM_ATTR gpio_isr_handler(void *arg) {
    pwm_counter++;
    if (pwm_counter >= 1000) {
        seconds++;
        pwm_counter = 0;
    }
}

void app_main(void) {
    // ------------------------------------------------------------------------
    // Initialize UART0 for communication at 115200 baud.
    // ------------------------------------------------------------------------
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0);

    // ------------------------------------------------------------------------
    // Configure the PWM timer:
    // A 1 kHz PWM with 16-bit resolution.
    // With a 99% duty cycle the PWM output will have one falling edge per period.
    // ------------------------------------------------------------------------
    pwm_timer_config_t timer_cfg = {
        .timer_num = LEDC_TIMER_0,
        .frequency_hz = 1000,
        .resolution_bit = LEDC_TIMER_16_BIT
    };
    pwm_timer_init(&timer_cfg);

    // ------------------------------------------------------------------------
    // Configure the PWM channel.
    // This channel outputs on PWM_GPIO. With a duty cycle of 99%, there will 
    // be a clear falling edge each cycle for the ISR to detect.
    // ------------------------------------------------------------------------
    pwm_channel_t pwm_ch = {
        .channel = LEDC_CHANNEL_0,
        .gpio_num = PWM_GPIO,
        .duty_percent = 99
    };
    pwm_channel_init(&pwm_ch, &timer_cfg);

    // ------------------------------------------------------------------------
    // Configure interrupt GPIO for falling edge detection.
    // In this example, INTERRUPT_GPIO receives the PWM output signal.
    // We set it as input (no pull-up) and configure a falling edge interrupt.
    // ------------------------------------------------------------------------
    isr_io_config(INTERRUPT_GPIO, true, false, false, GPIO_INTR_NEGEDGE);

    // Install the ISR service and attach the handler.
    gpio_install_isr_service(0); // Use default flags
    gpio_isr_handler_add(INTERRUPT_GPIO, gpio_isr_handler, NULL);

    // ------------------------------------------------------------------------
    // Timekeeping variables for formatted output.
    // ------------------------------------------------------------------------
    uint32_t last_printed_seconds = 0;
    uint32_t hours = 0;
    uint32_t minutes = 0;

    // Main loop: check for changes in the seconds counter and print time.
    while (1) {
        // If seconds have been updated by the ISR, update minutes/hours as needed.
        if (seconds != last_printed_seconds) {
            // Check if seconds should roll over
            if (seconds >= 60) {
                if (minutes >= 59) { // roll minute to 0 and increment hours
                    hours++;
                    minutes = 0;
                } else {
                    minutes++;
                }
                seconds = 0;
            }

            // Print the current time as hours:minutes:seconds over UART0.
            printf("%lu:%lu:%lu\n", hours, minutes, seconds);
            last_printed_seconds = seconds;
        }

        // Use a delay to reduce CPU usage in the main loop.
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
