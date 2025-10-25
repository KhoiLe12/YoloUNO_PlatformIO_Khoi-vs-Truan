#include "led_blinky.h"
#include "temp_humi_monitor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Strict setter: must take mutex; no fallback paths
void set_led_mode(LedMode mode) {
    configASSERT(xLedModeMutex != NULL);
    configASSERT(xLedModeChange != NULL);

    xSemaphoreTake(xLedModeMutex, portMAX_DELAY);
    bool changed = (g_ledMode != mode);
    g_ledMode = mode;
    xSemaphoreGive(xLedModeMutex);

    if (changed) {
        // wake the LED task immediately
        xSemaphoreGive(xLedModeChange);
    }
}

void led_blinky(void *pvParameters){
    pinMode(LED_GPIO, OUTPUT);

    // Ensure sync objects exist
    configASSERT(xLedModeMutex != NULL);
    configASSERT(xLedModeChange != NULL);

    for (;;) {
        // Read mode under mutex (block until available)
        xSemaphoreTake(xLedModeMutex, portMAX_DELAY);
        LedMode mode = g_ledMode;
        xSemaphoreGive(xLedModeMutex);

        switch (mode) {
            case LED_OFF:
                digitalWrite(LED_GPIO, LOW);
                // Block until someone signals a mode change
                xSemaphoreTake(xLedModeChange, portMAX_DELAY);
                break;

            case LED_ON:
                digitalWrite(LED_GPIO, HIGH);
                // Block until mode changes
                xSemaphoreTake(xLedModeChange, portMAX_DELAY);
                break;

            case LED_BLINK:
            default:
                // ON phase (wake early on mode change)
                digitalWrite(LED_GPIO, HIGH);
                if (xSemaphoreTake(xLedModeChange, pdMS_TO_TICKS(LED_BLINK_ON_MS)) == pdTRUE) {
                    // mode changed -> immediately apply new mode
                    break;
                }
                // OFF phase
                digitalWrite(LED_GPIO, LOW);
                if (xSemaphoreTake(xLedModeChange, pdMS_TO_TICKS(LED_BLINK_OFF_MS)) == pdTRUE) {
                    break;
                }
                break;
        }
        // loop to re-read mode
    }
}
