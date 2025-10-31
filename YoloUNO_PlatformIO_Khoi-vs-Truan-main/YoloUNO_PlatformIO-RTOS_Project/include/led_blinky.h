#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>
#include <freertos/semphr.h>

#define LED_GPIO 48
void led_blinky(void *pvParameters);

typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK
} LedMode;

// shared mode + sync (defined in main.cpp)
extern volatile LedMode g_ledMode;
extern SemaphoreHandle_t xLedModeMutex;
extern SemaphoreHandle_t xLedModeChange;

// blink timing (adjust as needed)
#define LED_BLINK_ON_MS  200
#define LED_BLINK_OFF_MS 800

// optional temperature thresholds for demo logic
#define TEMP_LOW_C   26.0
#define TEMP_HIGH_C  29.0

// convenience setter: safely change mode and wake LED task
void set_led_mode(LedMode mode);

#endif
