#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"

void temp_humi_monitor(void *pvParameters);
float get_latest_temperature(void);
float get_latest_humidity(void);

#endif
