#ifndef MOTOR_SERVO_H
#define MOTOR_SERVO_H

#include <Arduino.h>

// Motor servo control
void initMotorServo(QueueHandle_t queue, SemaphoreHandle_t sem);

#endif
