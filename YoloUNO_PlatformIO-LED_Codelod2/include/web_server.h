#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

// Access Point mode
void initWebServer(QueueHandle_t queue, SemaphoreHandle_t sem);

#endif
