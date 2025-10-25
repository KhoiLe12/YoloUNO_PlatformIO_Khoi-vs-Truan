#include "global.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "mainserver.h"
#include "tinyml.h"
#include <Wire.h>
#include <freertos/semphr.h>

// define shared mode and handles
volatile LedMode g_ledMode = LED_OFF;
SemaphoreHandle_t xLedModeMutex = NULL;
SemaphoreHandle_t xLedModeChange = NULL;
// NeoPixel color mode shared objects
volatile NeoMode g_neoMode = NEO_OFF;
SemaphoreHandle_t xNeoModeMutex = NULL;
SemaphoreHandle_t xNeoModeChange = NULL;

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(100));

  const int SDA_PIN = 11;
  const int SCL_PIN = 12;
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Serial.println("I2C Initialized.");

  // create synchronization objects BEFORE creating tasks
  xLedModeMutex = xSemaphoreCreateMutex();
  xLedModeChange = xSemaphoreCreateBinary();
  xNeoModeMutex = xSemaphoreCreateMutex();
  xNeoModeChange = xSemaphoreCreateBinary();
  if (xLedModeMutex == NULL || xLedModeChange == NULL ||
      xNeoModeMutex == NULL || xNeoModeChange == NULL) {
    Serial.println("Failed to create semaphores");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // create tasks
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 2048, NULL, 2, NULL);
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  // xTaskCreate(main_server_task, "Task Main Server", 2048, NULL, 2, NULL);
  //xTaskCreate(tiny_ml_task, "Tiny ML Task", 4096, NULL, 2, NULL);
}

void loop() { 

}

