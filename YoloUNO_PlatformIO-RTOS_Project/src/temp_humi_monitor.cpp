#include "temp_humi_monitor.h"
#include "led_blinky.h"  // LedMode, set_led_mode, TEMP_*_C
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

static volatile float latest_temperature = NAN;
static volatile float latest_humidity = NAN;

float get_latest_temperature(void) { return latest_temperature; }
float get_latest_humidity(void)    { return latest_humidity; }

void temp_humi_monitor(void *pvParameters){
    dht20.begin();

    while (1){
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity    = dht20.getHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        latest_temperature = temperature;
        latest_humidity    = humidity;

        // 3-state policy using thresholds from led_blinky.h
        if (temperature >= TEMP_HIGH_C) {
            set_led_mode(LED_ON);
        } else if (temperature >= TEMP_LOW_C) {
            set_led_mode(LED_BLINK);
        } else {
            set_led_mode(LED_OFF);
        }


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
