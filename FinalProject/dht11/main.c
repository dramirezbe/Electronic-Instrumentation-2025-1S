#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "dht11.h"

void app_main()
{
    DHT11_init(GPIO_NUM_27);

    while(1) {
        printf("Temperature is %.1f \n", DHT11_read().temperature);
        printf("Humidity is %.1f\n", DHT11_read().humidity);
        printf("Status code is %d\n", DHT11_read().status);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}