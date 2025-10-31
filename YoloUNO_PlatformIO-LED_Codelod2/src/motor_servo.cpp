#include "motor_servo.h"
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

#define SERVO_PIN 18
#define NEO_PIN 45
#define NEO_COUNT 1

Servo myServo;
Adafruit_NeoPixel pixels(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

QueueHandle_t qMotorServo;
SemaphoreHandle_t semServoMode;

void setColor(int angle) {
  if (angle == 0) pixels.setPixelColor(0, pixels.Color(255, 0, 0));     // Red
  else if (angle == 90) pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green
  else if (angle == 180) pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Blue
  pixels.show();
}

void motorTask(void *pvParameters) {
  int angle;
  while (true) {
    if (xQueueReceive(qMotorServo, &angle, portMAX_DELAY) == pdTRUE) {
      myServo.write(angle);
      setColor(angle);
    }
  }
}

void initMotorServo(QueueHandle_t queue, SemaphoreHandle_t sem) {
  qMotorServo = queue;
  semServoMode = sem;
  myServo.attach(SERVO_PIN);
  pixels.begin();
  setColor(90);
  xTaskCreatePinnedToCore(motorTask, "MotorServo", 4096, NULL, 1, NULL, 1);
}
