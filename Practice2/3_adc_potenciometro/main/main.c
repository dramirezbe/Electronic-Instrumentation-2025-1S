#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// Configuración del ADC basada en tu hardware
#define ADC_CHANNEL      ADC1_CHANNEL_7   // Ejemplo: GPIO35 en muchos ESP32
#define ADC_ATTENUATION  ADC_ATTEN_DB_11  // 11 dB de atenuación, máx ~3.9V
#define ADC_WIDTH        ADC_WIDTH_BIT_12 // Resolución de 12 bits (0-4095)
#define DEFAULT_VREF     1100             // Vref por defecto en mV (calibrar si es posible)

void app_main(void) {
    // Configurar el ADC1 con el ancho de bits y la atenuación deseada
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTENUATION);

    // Caracterizar el ADC para mejorar la conversión precisa de voltajes
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTENUATION, ADC_WIDTH, DEFAULT_VREF, &adc_chars);

    while (1) {
        // Leer valor bruto del ADC
        int raw_reading = adc1_get_raw(ADC_CHANNEL);

        // Convertir lectura a voltaje en mV usando la calibración
        uint32_t voltage = esp_adc_cal_raw_to_voltage(raw_reading, &adc_chars);

        // Imprimir valores en UART (Serial Plotter de Arduino)
        printf("%d\n", raw_reading);

        // Esperar 100ms entre lecturas
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
