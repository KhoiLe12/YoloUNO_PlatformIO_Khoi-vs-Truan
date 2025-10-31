#include "global.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "mainserver.h"
#include "tinyml.h"
#include "servo_controller.h"
#include "lcd_display.h"
#include <Wire.h>
#include <freertos/semphr.h>
#include "i2c_bus.h"
#include "esp_system.h"

// define shared mode and handles
volatile LedMode g_ledMode = LED_OFF;
SemaphoreHandle_t xLedModeMutex = NULL;
SemaphoreHandle_t xLedModeChange = NULL;
// NeoPixel color mode shared objects
volatile NeoMode g_neoMode = NEO_OFF;
SemaphoreHandle_t xNeoModeMutex = NULL;
SemaphoreHandle_t xNeoModeChange = NULL;

// Global I2C bus mutex
SemaphoreHandle_t xI2CMutex = NULL;

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(100));
  // Print last reset reason to help diagnose unexpected reboots
  esp_reset_reason_t rr = esp_reset_reason();
  Serial.printf("Reset reason: %d\r\n", (int)rr);

  const int SDA_PIN = 11;
  const int SCL_PIN = 12;
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Serial.println("I2C Initialized.");

  // Quick I2C scan to help diagnose sensor connectivity
  if (i2c_lock(pdMS_TO_TICKS(1000))) {
    Serial.println("I2C scan start...");
    int found = 0;
    for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
      Wire.beginTransmission(addr);
      uint8_t err = Wire.endTransmission();
      if (err == 0) {
        Serial.print(" - I2C device at 0x");
        if (addr < 16) Serial.print('0');
        Serial.println(addr, HEX);
        found++;
      }
      vTaskDelay(1);
    }
    if (found == 0) Serial.println("No I2C devices found. Check wiring/power/pull-ups.");
    else Serial.printf("%d I2C device(s) found. Expect DHT20 at 0x38.\r\n", found);
    i2c_unlock();
  }

  // create synchronization objects BEFORE creating tasks
  xLedModeMutex = xSemaphoreCreateMutex();
  xLedModeChange = xSemaphoreCreateBinary();
  xNeoModeMutex = xSemaphoreCreateMutex();
  xNeoModeChange = xSemaphoreCreateBinary();
  xI2CMutex = xSemaphoreCreateMutex();
  if (xLedModeMutex == NULL || xLedModeChange == NULL ||
      xNeoModeMutex == NULL || xNeoModeChange == NULL ||
      xI2CMutex == NULL) {
    Serial.println("Failed to create semaphores");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // create tasks
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 2048, NULL, 2, NULL);
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  xTaskCreate(main_server_task, "Task Main Server", 4096, NULL, 2, NULL);
  xTaskCreate(tiny_ml_task, "Tiny ML Task", 4096, NULL, 2, NULL);
  xTaskCreate(servo_controller_task, "Task Servo Controller", 4096, NULL, 2, NULL);
  // Give the LCD task a larger stack and pin to core 0 to avoid contention with WiFi (core 1)
  xTaskCreatePinnedToCore(lcd_display_task, "Task LCD Display", 4096, NULL, 1, NULL, 0);
}

void loop() { 

}

