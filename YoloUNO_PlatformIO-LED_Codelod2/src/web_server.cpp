#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);
QueueHandle_t qMotor;
SemaphoreHandle_t semMode;
bool useML = false;

void handleRoot() {
  String html = "<html><body><h1>ESP32 Control</h1>";
  html += "<form action='/left'><button>Left</button></form>";
  html += "<form action='/center'><button>Center</button></form>";
  html += "<form action='/right'><button>Right</button></form>";
  html += "<p>Mode: " + String(useML ? "ML" : "Web") + "</p>";
  html += "<form action='/toggle'><button>Switch Mode</button></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLeft() { int val = 0; xQueueSend(qMotor, &val, 0); server.sendHeader("Location", "/"); server.send(303); }
void handleCenter() { int val = 90; xQueueSend(qMotor, &val, 0); server.sendHeader("Location", "/"); server.send(303); }
void handleRight() { int val = 180; xQueueSend(qMotor, &val, 0); server.sendHeader("Location", "/"); server.send(303); }

void handleToggle() {
  useML = !useML;
  if (useML) xSemaphoreTake(semMode, portMAX_DELAY);
  else xSemaphoreGive(semMode);
  server.sendHeader("Location", "/");
  server.send(303);
}

void webTask(void *pvParameters) {
  while (true) {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void initWebServer(QueueHandle_t queue, SemaphoreHandle_t sem) {
  qMotor = queue;
  semMode = sem;
  WiFi.softAP("ESP32_AP", "12345678");
  WiFi.softAPConfig(IPAddress(192,168,20,11), IPAddress(192,168,20,11), IPAddress(255,255,255,0));

  Serial.println("Access Point: 192.168.20.11");

  server.on("/", handleRoot);
  server.on("/left", handleLeft);
  server.on("/center", handleCenter);
  server.on("/right", handleRight);
  server.on("/toggle", handleToggle);
  server.begin();

  xTaskCreatePinnedToCore(webTask, "WebServer", 4096, NULL, 1, NULL, 0);
}
