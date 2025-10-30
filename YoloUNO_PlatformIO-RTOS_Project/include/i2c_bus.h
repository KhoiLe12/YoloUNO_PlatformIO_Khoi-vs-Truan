#ifndef __I2C_BUS_MUTEX__
#define __I2C_BUS_MUTEX__

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Global I2C mutex handle defined in main.cpp
extern SemaphoreHandle_t xI2CMutex;

// Convenience helpers to guard Wire transactions
static inline bool i2c_lock(TickType_t ticks = portMAX_DELAY) {
    return (xI2CMutex != NULL) && (xSemaphoreTake(xI2CMutex, ticks) == pdTRUE);
}

static inline void i2c_unlock() {
    if (xI2CMutex) {
        xSemaphoreGive(xI2CMutex);
    }
}

#endif
