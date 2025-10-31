#ifndef ML_MODULE_H
#define ML_MODULE_H

#include <Arduino.h>

// Machine Learning module initialization
void initML(QueueHandle_t queue, SemaphoreHandle_t sem);

#endif
