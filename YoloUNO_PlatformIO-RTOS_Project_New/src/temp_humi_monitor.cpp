#include "temp_humi_monitor.h"
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
        bool ok = dht20.begin();
        uint8_t addr = dht20.getAddress();
        Serial.printf("DHT20 begin() %s at 0x%02X\r\n", ok?"OK":"FAILED", addr);
        if (!ok) {
            Serial.println("Hint: Expect I2C address 0x38. Check SDA/SCL pins, wiring, power.");
        }
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
            // On read failure, print debug and try next cycle
            Serial.printf("DHT20 read error %d\r\n", rd);
            // If the bus is saturated by other tasks or WiFi starting up, wait a bit longer
            vTaskDelay(pdMS_TO_TICKS(1500));
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


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

