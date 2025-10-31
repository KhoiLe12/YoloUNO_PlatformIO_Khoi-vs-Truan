#include <Arduino.h>
#include "web_server.h"
#include "motor_servo.h"
#include "lcd_display.h"
#include "ml_module.h"

QueueHandle_t motorCommandQueue;   // Gửi lệnh từ Web/ML -> Servo
SemaphoreHandle_t modeSwitch;      // Điều khiển Web mode / ML mode

void setup() {
  Serial.begin(115200);
  delay(1000);

  motorCommandQueue = xQueueCreate(5, sizeof(int)); // 0, 90, 180
  modeSwitch = xSemaphoreCreateBinary();
  xSemaphoreGive(modeSwitch); // default = Web control mode

  initWebServer(motorCommandQueue, modeSwitch);
  initMotorServo(motorCommandQueue, modeSwitch);
  initLCD();
  initML(motorCommandQueue, modeSwitch);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
