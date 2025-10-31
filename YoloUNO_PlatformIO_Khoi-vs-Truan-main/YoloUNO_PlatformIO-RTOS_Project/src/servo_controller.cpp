#include "servo_controller.h"
#include "neo_blinky.h"
#include "tinyml.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <ESP32Servo.h>

// Servo control using ESP32Servo library with smooth transitions
static SemaphoreHandle_t xServoMutex = NULL;
static volatile ControlMode g_mode = MODE_MANUAL;
static volatile ServoPos g_pos = SERVO_CENTER;
static volatile int g_angle = 90;

static Servo g_servo;

static inline int clamp_angle(int a) {
  if (a < 0) return 0; if (a > 180) return 180; return a;
}

static void write_angle_immediate(int angle) {
  angle = clamp_angle(angle);
  g_servo.write(angle);
  g_angle = angle;
}

static void gradually_move_to(int targetAngle) {
  targetAngle = clamp_angle(targetAngle);
  int current = g_angle;
  if (current == targetAngle) return;
  int step = (targetAngle > current) ? 1 : -1;
  while (current != targetAngle) {
    current += step;
    g_servo.write(current);
    g_angle = current;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void apply_pos_to_servo_and_neo(ServoPos pos) {
  int angle = 90;
  NeoMode nm = NEO_OFF;
  switch (pos) {
    case SERVO_LEFT:   angle = 0;   nm = NEO_HIGH;   break; // Left = Red
    case SERVO_CENTER: angle = 90;  nm = NEO_MEDIUM; break; // Center = Green
    case SERVO_RIGHT:  angle = 180; nm = NEO_LOW;    break; // Right = Blue
  }
  gradually_move_to(angle);
  set_neo_mode(nm);
}

void servo_set_mode(ControlMode mode){
  if (!xServoMutex) return;
  xSemaphoreTake(xServoMutex, portMAX_DELAY);
  g_mode = mode;
  xSemaphoreGive(xServoMutex);
}

ControlMode servo_get_mode(){
  return g_mode;
}

void servo_set_manual(ServoPos pos){
  if (!xServoMutex) return;
  xSemaphoreTake(xServoMutex, portMAX_DELAY);
  g_pos = pos;
  xSemaphoreGive(xServoMutex);
}

ServoPos servo_get_position(){
  return g_pos;
}

int servo_get_angle(){
  return g_angle;
}

void servo_controller_task(void* pvParameters){
  // Give system time to stabilize (WiFi/AP start, power rails)
  vTaskDelay(pdMS_TO_TICKS(2000));
  // Init ESP32Servo
  g_servo.setPeriodHertz(SERVO_PWM_FREQ_HZ);
  // Attach with explicit min/max pulse widths
  g_servo.attach(SERVO_PIN, SERVO_US_MIN, SERVO_US_MAX);

  xServoMutex = xSemaphoreCreateMutex();

  // Set initial center after short delay to avoid brownout on power-up
  write_angle_immediate(90);
  set_neo_mode(NEO_MEDIUM);

  ControlMode lastMode = g_mode;
  ServoPos lastPos = g_pos;
  int lastAngle = g_angle;
  float lastScoreBucket = -1.0f;

  for(;;){
    // Snapshot
    xSemaphoreTake(xServoMutex, portMAX_DELAY);
    ControlMode mode = g_mode;
    ServoPos pos = g_pos;
    xSemaphoreGive(xServoMutex);

    if (mode == MODE_ML) {
      float score = get_latest_ml_score();
      // Map score to Left/Center/Right buckets; use -1 if NAN to skip
      float bucket = -1.0f;
      if (!isnan(score)) {
        if (score <= 0.333f)      pos = SERVO_LEFT;
        else if (score <= 0.666f) pos = SERVO_CENTER;
        else                      pos = SERVO_RIGHT;
        bucket = (float)pos;
      }
      if (bucket != lastScoreBucket) {
        apply_pos_to_servo_and_neo(pos);
        lastScoreBucket = bucket;
        lastAngle = g_angle;
        lastPos = pos;
      }
    } else { // Manual
      if (pos != lastPos) {
        apply_pos_to_servo_and_neo(pos);
        lastPos = pos;
        lastAngle = g_angle;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
