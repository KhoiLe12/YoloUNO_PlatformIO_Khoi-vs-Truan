#include "neo_blinky.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Setter implementation: change mode under mutex and signal task
void set_neo_mode(NeoMode mode) {
    configASSERT(xNeoModeMutex != NULL);
    configASSERT(xNeoModeChange != NULL);

    xSemaphoreTake(xNeoModeMutex, portMAX_DELAY);
    bool changed = (g_neoMode != mode);
    g_neoMode = mode;
    xSemaphoreGive(xNeoModeMutex);

    if (changed) {
        xSemaphoreGive(xNeoModeChange);
    }
}

void neo_blinky(void *pvParameters){
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.setBrightness(NEO_BRIGHTNESS);
    strip.clear();
    strip.show();

    // Ensure sync objects exist
    configASSERT(xNeoModeMutex != NULL);
    configASSERT(xNeoModeChange != NULL);

    for (;;) {
        // Read mode under mutex
        xSemaphoreTake(xNeoModeMutex, portMAX_DELAY);
        NeoMode mode = g_neoMode;
        xSemaphoreGive(xNeoModeMutex);

        uint32_t color = strip.Color(0, 0, 0);
        switch (mode) {
            case NEO_OFF:
                color = strip.Color(0, 0, 0);
                break;
            case NEO_LOW:     // low humidity -> Blue
                color = strip.Color(0, 0, 255);
                break;
            case NEO_MEDIUM:  // medium humidity -> Green
                color = strip.Color(0, 255, 0);
                break;
            case NEO_HIGH:    // high humidity -> Red
            default:
                color = strip.Color(255, 0, 0);
                break;
        }

        for (uint16_t i = 0; i < LED_COUNT; ++i) {
            strip.setPixelColor(i, color);
        }
        strip.show();

        // Block until someone changes mode, similar to led_blinky
        xSemaphoreTake(xNeoModeChange, portMAX_DELAY);
    }
}
