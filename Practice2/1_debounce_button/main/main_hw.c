
#include "tim_ch_duty.h"
#include "io_utils.h"

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define BTN                   GPIO_NUM_25
#define DEBOUNCE_TIME_MS 20 //Software debounce


#define LED GPIO_NUM_2

static QueueHandle_t gpio_evt_queue = NULL;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
static void btn_task(void* arg)
{
    uint32_t io_num; //gpio_num
    TickType_t last_press_time = 0;
    uint8_t level = gpio_get_level(LED);
    
    while(1) {

        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            TickType_t now = xTaskGetTickCount(); //Systick

            //Debounce
            if (is_debounced(now, last_press_time, DEBOUNCE_TIME_MS)) {
                last_press_time = now;

                if(io_num == BTN) {
                    level = !level;
                    gpio_set_level(LED, level);
                }
            }
        }
    }
}



void app_main(void) {

    //gpio_num_t io_num, bool is_input, bool pull_up, bool open_drain, gpio_int_type_t isr_type
    isr_io_config(BTN, true, true, false, GPIO_INTR_NEGEDGE);
    //gpio_num_t io_num, bool is_input, bool pull_up, bool open_drain
    io_config(LED, false, false, false);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN, gpio_isr_handler, (void*) BTN);


    xTaskCreatePinnedToCore(btn_task, "btn_task", 2048, NULL, 4, NULL, 0);

    printf("-------------------Done-------------------");

}