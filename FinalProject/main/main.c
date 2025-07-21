/**
 * @file main.c
 * @author David Ramírez Betancourth
 * @details
 */

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/queue.h"

#include "adc_utils.h"
#include "tim_ch_duty.h"
#include "io_utils.h"

#include "wifi_app.h"
#include "http_server.h"

#include <math.h>

#include <ssd1306.h>

//-------------------ADC-----------------------
#define NTC_ADC_CH ADC_CHANNEL_7 //IO 35
#define LM35_ADC_CH ADC_CHANNEL_6 //IO 34

#define ADC_UNIT   ADC_UNIT_1
#define ADC_ATTEN  ADC_ATTEN_DB_12

#define LM35_ADC_DATA_TYPE true
#define NTC_DATA_TYPE false

//-------------------UART----------------------
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)


//-------------------PWM----------------------
#define PWM_FREQ_HZ 1000
#define PWM_PIN = GPIO_NUM_27

//-----------------------------------------Struct----------------------------------------

typedef struct {
    float value;
    bool type;
} adc_type_data_t;

//---------------------------------------Global Vars ------------------------------------


static QueueHandle_t adc_data_queue;
static QueueHandle_t uart_rx_queue;

// Mutex for shared ADC resource
SemaphoreHandle_t xMutex;

static uint8_t uart_rx_buffer[RD_BUF_SIZE];

//------------------------------------Config Peripherals-------------------------------------

// LEDC Timer and Channels configuration
pwm_timer_config_t timer = {.frequency_hz = PWM_FREQ_HZ, .resolution_bit = LEDC_TIMER_10_BIT, .timer_num = LEDC_TIMER_0};

// ADC Configurations and Handles
adc_config_t ntc_adc_conf = {
    .unit_id = ADC_UNIT,
    .channel = NTC_ADC_CH,
    .atten = ADC_ATTEN,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_channel_handle_t ntc_adc_handle = NULL;

adc_config_t lm35_adc_conf = {
    .unit_id = ADC_UNIT,
    .channel = LM35_ADC_CH,
    .atten = ADC_ATTEN,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_channel_handle_t lm35_adc_handle = NULL;

//-----------------------------------Helper Functions------------------------------------------

double ntc_temperature;
void ntc_task(void *arg) {

    adc_type_data_t ntc_item;
    ntc_item.type = NTC_DATA_TYPE;
    ntc_item.value = 0;

    // These constants should be specific to your NTC thermistor
    const float beta = 10000.0;     // Beta value for your NTC
    const float R0 = 10000.0;      // Resistance of NTC at T0 (e.g., 10k Ohms at 25°C)
    const float T0 = 298.15;       // Reference temperature in Kelvin (25°C + 273.15)
    const float R2 = 1000.0;       // Resistor in voltage divider (1k Ohm)

    const float Vi = 4.8;          // Input voltage to the voltage divider

    float Vo = 0;                  // Voltage measured across R2
    float Rt = 0;                  // Resistance of the NTC
    float T = 0;                   // Temperature in Kelvin
    float final_T = 0;             // Temperature in Celsius

    // 1. Configure ADC
    set_adc(&ntc_adc_conf, &ntc_adc_handle); // Assuming ntc_adc_conf and ntc_adc_handle are defined globally

    int raw_val = 0;
    int voltage_mv = 0;

    while (1) {

        xSemaphoreTake(xMutex, portMAX_DELAY); // Acquire mutex for ADC

        get_raw_data(ntc_adc_handle, &raw_val);
        raw_to_voltage(ntc_adc_handle, raw_val, &voltage_mv);

        Vo = (float)voltage_mv / 1000.0f; // Convert mV to Volts (voltage across R2)

        // CORRECTED FORMULA FOR Rt (NTC Resistance)
        // Given: Vo is measured across R2, R1 is NTC, Vi is total input
        // Rt = R2 * (Vi - Vo) / Vo
        // Make sure (Vi - Vo) is not negative or zero if Vo approaches Vi,
        // which implies issues with the circuit or ADC range.
        if (Vo > 0 && Vi > Vo) { // Avoid division by zero and negative resistance
            Rt = (R2 * (Vi - Vo)) / Vo;
        } else {
            // Handle error or set a default/error value for Rt
            // For now, we'll just skip the calculation and print an error.
            printf("Error: Invalid Vo (%.2fV) or Vi (%.2fV) for Rt calculation.\r\n", Vo, Vi);
            xSemaphoreGive(xMutex); // Release mutex
            vTaskDelay(pdMS_TO_TICKS(500));
            continue; // Skip to next iteration
        }


        // Steinhart-Hart equation (Beta approximation) - this part was already correct
        T = (beta * T0) / (logf(Rt / R0) * T0 + beta);
        final_T = T - 273.15f; // Convert Kelvin to Celsius

        ntc_temperature = final_T; // Send to http as global var (if used)

        // VERBOSE
        // printf("Raw: %d, Voltage_mv: %d, Vo: %.2fV, Rt: %.2f Ohms, T_Kelvin: %.2f K, T_Celsius: %.2f °C\r\n",
        //        raw_val, voltage_mv, Vo, Rt, T, final_T);

        ntc_item.value = final_T;
        // Ensure the queue exists and is handled correctly by the consumer task
        if (xQueueSend(adc_data_queue, &ntc_item, (TickType_t)pdMS_TO_TICKS(10)) != pdPASS) {
            printf("Failed to send NTC data to queue.\r\n");
        }

        xSemaphoreGive(xMutex); // Release mutex

        vTaskDelay(pdMS_TO_TICKS(51)); // Delay
    }
}

void lm35_task(void *arg) {
    
    adc_type_data_t lm35_item;
    lm35_item.type = LM35_ADC_DATA_TYPE;
    lm35_item.value = 0;

    float final_T = 0; // Temperature in Celsius

    // 1. Configure ADC
    set_adc(&lm35_adc_conf, &lm35_adc_handle);

    int raw_val = 0;
    int voltage_mv = 0;

    while (1) {
        
        xSemaphoreTake(xMutex, portMAX_DELAY); // Acquire mutex for ADC
        get_raw_data(lm35_adc_handle, &raw_val);
        raw_to_voltage(lm35_adc_handle, raw_val, &voltage_mv);

        final_T = (float)voltage_mv / 10.0f;
        
        //VERBOSE
        //printf("T: %.2f °C\r\n", final_T);

        lm35_item.value = final_T;
        xQueueSend(adc_data_queue, &lm35_item, (TickType_t)pdMS_TO_TICKS(10)); // Send the full struct
        xSemaphoreGive(xMutex); // Release mutex

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Read UART task
void uart_rx_task(void *arg) {
    //Config UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, RD_BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_rx_queue, 0);

    printf("UART initialized.\r\n");

    uart_event_t event;
    while(1) {
        //wait UART RX
        if(xQueueReceive(uart_rx_queue, (void * )&event, (TickType_t)pdMS_TO_TICKS(10))) {
        
            uart_read_bytes(UART_NUM, uart_rx_buffer, event.size, portMAX_DELAY);
            uart_rx_buffer[event.size] = '\0';

            char *str_buffer = (char *)uart_rx_buffer;

			//Handle str_buffer here
            printf("UART RX: %s\r\n", str_buffer);
            
        }
    }
}

void adc_task(void *arg) {
    adc_type_data_t adc_item;
    adc_item.type = NTC_DATA_TYPE;
    adc_item.value = 0;
    float current_ntc = 0;
    float current_lm35 = 0;
    float new_ntc = 0;
    float new_lm35 = 0;

    TickType_t last_display_time = xTaskGetTickCount();

    float diff = 0;

    char wind[20];
    char ambient[20];

    while(1) {
        
        if (xQueueReceive(adc_data_queue, &adc_item, portMAX_DELAY)) {
            
            if(adc_item.type) {
                //printf("LM35: %.2f °C\r\n", adc_item.value);
                new_lm35 = adc_item.value;

                

            } else {
                //printf("NTC: %.2f °C\r\n", adc_item.value);
                new_ntc = adc_item.value;
                
            }
            current_lm35 = new_lm35;
            current_ntc = new_ntc;
            diff = fabs(current_ntc - current_lm35);
            sprintf(wind, "%.2f Km/h", diff);
            sprintf(ambient, "%.2f °C", current_lm35);
            
            // Update display only if 1 second has passed
            if (xTaskGetTickCount() - last_display_time >= pdMS_TO_TICKS(1000)) {
                ssd1306_clear();
                ssd1306_print_str(18,0,"Wind Speed:", false);
                ssd1306_print_str(28,15,wind, false);
                ssd1306_print_str(18,30,"Ambient Temp:", false);
                ssd1306_print_str(28,45,ambient, false);
                ssd1306_display();
                last_display_time = xTaskGetTickCount();
            }

        }
        
         
    }


}


void app_main(void)
{

    //MUTEX
    xMutex = xSemaphoreCreateMutex();
    printf("Mutex created successfully.\r\n");

    //PWM
    pwm_timer_init(&timer);
    printf("Timer Initialized. \r\n");

    //ADC
    adc_data_queue = xQueueCreate(10, sizeof(adc_type_data_t));

    // Initialize NVS
	// esp_err_t ret = nvs_flash_init();
	// if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	// {
	// 	ESP_ERROR_CHECK(nvs_flash_erase());
	// 	ret = nvs_flash_init();
	// }
	// ESP_ERROR_CHECK(ret);
	
	// Start Wifi
	//wifi_app_start();

    init_ssd1306();
	
    xTaskCreate(uart_rx_task, "uart_rx_task", 2048, NULL, 5, NULL);
    xTaskCreate(ntc_task, "ntc_task", 4096, NULL, 4, NULL);
    xTaskCreate(lm35_task, "lm35_task", 4096, NULL, 4, NULL);
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 4, NULL);

}