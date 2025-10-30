#include "temp_humi_monitor.h"
#include "led_blinky.h"  // LedMode, set_led_mode, TEMP_*_C
#include "neo_blinky.h"  // NeoMode, set_neo_mode, HUM_* thresholds
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "i2c_bus.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

static volatile float latest_temperature = NAN;
static volatile float latest_humidity = NAN;

float get_latest_temperature(void) { return latest_temperature; }
float get_latest_humidity(void)    { return latest_humidity; }

void temp_humi_monitor(void *pvParameters){
    // Guard sensor init on the I2C bus
    if (i2c_lock()) {
        dht20.begin();
        i2c_unlock();
    }

    while (1){
        // Serialize the full sensor read sequence
        int rd = -1;
        if (i2c_lock()) {
            rd = dht20.read();
            i2c_unlock();
        }

        if (rd < 0) {
            // On read failure, wait a bit and try next cycle
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
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

        // Map humidity to NeoPixel 3-state color via semaphore pattern
        if (humidity < HUM_LOW_PCT) {
            set_neo_mode(NEO_LOW);
        } else if (humidity <= HUM_HIGH_PCT) {
            set_neo_mode(NEO_MEDIUM);
        } else {
            set_neo_mode(NEO_HIGH);
        }


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

