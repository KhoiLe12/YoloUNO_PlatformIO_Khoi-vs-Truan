// T giả lập nên để m dễ hơn ha :)))
#include "ml_module.h"

QueueHandle_t qMotorML;
SemaphoreHandle_t semML;

void mlTask(void *pvParameters) {
  float ml_output;
  int mappedAngle;

  while (true) {
    if (xSemaphoreTake(semML, 0) == pdTRUE) { // if ML mode active
      ml_output = random(0, 100) / 100.0; // fake ML output 0–1

      if (ml_output < 0.33) mappedAngle = 0;
      else if (ml_output < 0.66) mappedAngle = 90;
      else mappedAngle = 180;

      xQueueSend(qMotorML, &mappedAngle, 0);
      Serial.printf("ML Output: %.2f -> %d\n", ml_output, mappedAngle);
      xSemaphoreGive(semML);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void initML(QueueHandle_t queue, SemaphoreHandle_t sem) {
  qMotorML = queue;
  semML = sem;
  xTaskCreatePinnedToCore(mlTask, "ML", 4096, NULL, 1, NULL, 1);
}
