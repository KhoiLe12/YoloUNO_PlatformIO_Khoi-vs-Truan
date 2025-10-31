#ifndef __SERVO_CONTROLLER__
#define __SERVO_CONTROLLER__

#include <Arduino.h>

// Default servo pin (change if needed for your board wiring)
#ifndef SERVO_PIN
#define SERVO_PIN 38
#endif

// 50Hz typical hobby servo
#ifndef SERVO_PWM_FREQ_HZ
#define SERVO_PWM_FREQ_HZ 50
#endif

// Pulse widths in microseconds for 0 and 180 degrees
#ifndef SERVO_US_MIN
#define SERVO_US_MIN 500
#endif
#ifndef SERVO_US_MAX
#define SERVO_US_MAX 2500
#endif

// Control modes
typedef enum {
  MODE_MANUAL = 0,
  MODE_ML     = 1
} ControlMode;

// Logical positions
typedef enum {
  SERVO_LEFT = 0,
  SERVO_CENTER = 1,
  SERVO_RIGHT = 2
} ServoPos;

// Task entry
void servo_controller_task(void* pvParameters);

// Control APIs (thread-safe)
void servo_set_mode(ControlMode mode);
ControlMode servo_get_mode();

void servo_set_manual(ServoPos pos);
ServoPos servo_get_position();

int servo_get_angle(); // degrees 0-180

#endif
