#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"
#include "i2c_bus.h"
#include "lcd_display.h"

// Use the same address/dimensions style as elsewhere in the project
// Adjust address if your LCD backpack differs (e.g., 0x27 or 0x3F)
static LiquidCrystal_I2C g_lcd(33, 16, 2);

static const char* dir_from_score(float score){
  if (isnan(score)) return "--";
  if (score <= 0.333f) return "Left";
  if (score <= 0.666f) return "Center";
  return "Right";
}

void lcd_display_task(void* pvParameters){
  // Initialize LCD under I2C mutex
  if (i2c_lock(pdMS_TO_TICKS(1000))) {
    g_lcd.begin();
    g_lcd.backlight();
    g_lcd.clear();
    g_lcd.setCursor(0,0);
    g_lcd.print("ESP32 LCD Ready");
    i2c_unlock();
  }

  static char lastL1[17] = {0};
  static char lastL2[17] = {0};

  for(;;){
    // Gather data
    float t = get_latest_temperature();
    float h = get_latest_humidity();
    float s = get_latest_ml_score();

    // Fallbacks
    if (isnan(t)) t = 0.0f;
    if (isnan(h)) h = 0.0f;

    // Compose lines (16 cols), use fixed buffers
    // Line1: T:24.6C H:55.2%
    char line1[17];
    int n1 = snprintf(line1, sizeof(line1), "T:%4.1fC H:%4.1f%%", (double)t, (double)h);
    if (n1 < 0) n1 = 0; if (n1 > 16) n1 = 16;
    for (int i=n1; i<16; ++i) line1[i] = ' ';
    line1[16] = '\0';

    // Line2: ML:0.45 Center
    const char* dir = dir_from_score(s);
    char scorebuf[8];
    if (isnan(s)) strcpy(scorebuf, "--"); else snprintf(scorebuf, sizeof(scorebuf), "%.2f", (double)s);
    char line2[17];
    int n2 = snprintf(line2, sizeof(line2), "ML:%-5s %-6s", scorebuf, dir);
    if (n2 < 0) n2 = 0; if (n2 > 16) n2 = 16;
    for (int i=n2; i<16; ++i) line2[i] = ' ';
    line2[16] = '\0';

    // Only write if changed to reduce flicker/bus traffic
    if (i2c_lock(pdMS_TO_TICKS(200))) {
      if (strncmp(line1, lastL1, 16) != 0) {
        g_lcd.setCursor(0,0);
        g_lcd.print(line1);
        memcpy(lastL1, line1, 17);
      }
      if (strncmp(line2, lastL2, 16) != 0) {
        g_lcd.setCursor(0,1);
        g_lcd.print(line2);
        memcpy(lastL2, line2, 17);
      }
      i2c_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
