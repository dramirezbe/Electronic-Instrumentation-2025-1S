/**
 * @file main.c
 * @author David Ramírez Betancourth
 * @brief Init uart 0, 115200, if the word RANDOM is read, then uart must write OK, and then send a random number within 0 to 9
 * @details 
 */

#include <stdio.h>
#include <string.h> // Required for strcmp
#include <stdlib.h> // Required for rand and srand
#include <time.h>   // Required for time (for srand)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

#include "adc_utils.h"
#include "tim_ch_duty.h"
#include "io_utils.h"

//-----------------UART---------------------------
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256 // Added TX buffer size
#define UART_QUEUE_LENGTH   20 // Queue for UART events/data


//-------------------Global Variables-------------------------

static QueueHandle_t uart_queue = NULL;    // Queue for UART events

//pwm_timer_config_t timer = {.frequency_hz = 1000, .resolution_bit = LEDC_TIMER_10_BIT, .timer_num = LEDC_TIMER_0};

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(UART_RX_BUFFER_SIZE);
    srand(time(NULL)); // Seed the random number generator

    for (;;) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            bzero(dtmp, UART_RX_BUFFER_SIZE);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_PORT, dtmp, event.size, portMAX_DELAY);
                    dtmp[event.size] = '\0'; // Null-terminate the received data
                    printf("Received: %s\n", dtmp);

                    if (strcmp((char *)dtmp, "RANDOM") == 0) {
                        uart_write_bytes(UART_PORT, "OK\r\n", strlen("OK\r\n"));
                        int random_num = rand() % 10; // Random number between 0 and 9
                        char num_str[2];
                        sprintf(num_str, "%d\r\n", random_num);
                        uart_write_bytes(UART_PORT, num_str, strlen(num_str));
                        printf("Sent: OK and %d\n", random_num);
                    }
                    break;
                // Add other UART event types if necessary for a more robust application
                case UART_FIFO_OVF:
                    printf("UART FIFO overflow!\n");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    printf("UART buffer full!\n");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;
                case UART_BREAK:
                    printf("UART break signal detected\n");
                    break;
                case UART_PARITY_ERR:
                    printf("UART parity error\n");
                    break;
                case UART_FRAME_ERR:
                    printf("UART frame error\n");
                    break;
                default:
                    printf("UART event type: %d\n", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    vTaskDelete(NULL);
}


void app_main(void)
{
    // pwm_timer_init(&timer); // Stay commented as per instructions

    // Initialize UART
    init_uart(UART_PORT, UART_BAUD_RATE, &uart_queue, UART_RX_BUFFER_SIZE);
    printf("UART initialized on port %d at %d baud\n", UART_PORT, UART_BAUD_RATE);

    uart_queue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(uart_event_t)); // Queue for UART events

    xTaskCreate(uart_event_task, "uart_event_task", 4098, NULL, 5, NULL);

    printf("-------------------Done-------------------\n");
}