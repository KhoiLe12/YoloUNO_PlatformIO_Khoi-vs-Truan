#ifndef __LCD_DISPLAY__
#define __LCD_DISPLAY__

#include <Arduino.h>

// FreeRTOS task to drive the 16x2 I2C LCD with DHT20 and TinyML data
void lcd_display_task(void* pvParameters);

#endif
