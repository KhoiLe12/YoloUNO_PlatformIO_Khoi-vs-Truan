#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <freertos/semphr.h>



#define NEO_PIN 45
#define LED_COUNT 1 

// Humidity thresholds (%RH) for color mapping
// < HUM_LOW_PCT  -> Low humidity color
// [HUM_LOW_PCT, HUM_HIGH_PCT] -> Medium humidity color
// > HUM_HIGH_PCT -> High humidity color
#define HUM_LOW_PCT   40.0f
#define HUM_HIGH_PCT  80.0f

// Optional overall brightness (0-255)
#ifndef NEO_BRIGHTNESS
#define NEO_BRIGHTNESS 32
#endif

void neo_blinky(void *pvParameters);

// Semaphore-based color state management (similar to led_blinky)
typedef enum {
	NEO_OFF = 0,
	NEO_LOW,     // below HUM_LOW_PCT
	NEO_MEDIUM,  // between HUM_LOW_PCT and HUM_HIGH_PCT
	NEO_HIGH     // above HUM_HIGH_PCT
} NeoMode;

// Shared mode and sync objects (defined in main.cpp)
extern volatile NeoMode g_neoMode;
extern SemaphoreHandle_t xNeoModeMutex;
extern SemaphoreHandle_t xNeoModeChange;

// Safely change Neo mode and notify the task
void set_neo_mode(NeoMode mode);


#endif
