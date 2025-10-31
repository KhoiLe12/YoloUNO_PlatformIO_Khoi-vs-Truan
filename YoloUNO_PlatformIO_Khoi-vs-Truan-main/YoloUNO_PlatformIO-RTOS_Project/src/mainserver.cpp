#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>
// Real sensor values and control
#include "temp_humi_monitor.h"
#include "servo_controller.h"
#include "tinyml.h"

bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;

WebServer server(80);

String ssid = "ESP32-TK";
String password = "12345678";
String wifi_ssid = "";
String wifi_password = "";

unsigned long connect_start_ms = 0;
bool connecting = false;

String mainPage() {
  // Initial values
  float temperature = get_latest_temperature();
  float humidity = get_latest_humidity();
  if (isnan(temperature)) temperature = 0.0f;
  if (isnan(humidity)) humidity = 0.0f;
  String mode = (servo_get_mode() == MODE_ML) ? "ML" : "Manual";
  int angle = servo_get_angle();
  ServoPos pos = servo_get_position();
  const char* posStr = (pos==SERVO_LEFT?"Left":(pos==SERVO_CENTER?"Center":"Right"));

  return String(
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>ESP32 Control</title>"
    "<style>"
      "body{font-family:Arial;margin:0;background:#f4f6f8;color:#222}"
      ".card{max-width:420px;margin:18px auto;background:#fff;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,.08);padding:18px}"
      ".row{display:flex;gap:10px;justify-content:center;margin:10px 0}"
      ".btn{padding:10px 14px;border:0;border-radius:8px;background:#e9ecef;cursor:pointer;font-size:16px}"
      ".btn.primary{background:#2563eb;color:#fff}"
      ".seg .btn{flex:1}"
      ".label{font-weight:bold}"
      ".switch{display:inline-flex;align-items:center;gap:8px}"
      ".wifi{float:right}"
    "</style></head><body>"
    "<div class='card'>"
      "<div class='row'><h2 style='margin:0'>ESP32 Control</h2><button class='btn primary wifi' onclick=\"window.location='/settings'\">WiFi</button></div>"
      "<div class='row'><div>"
        "<div><span class='label'>Temperature:</span> <span id='temp'>" + String(temperature) + "</span> &deg;C</div>"
        "<div><span class='label'>Humidity:</span> <span id='hum'>" + String(humidity) + "</span> %</div>"
      "</div></div>"
      "<div class='row switch'>"
        "<span class='label'>Mode:</span>"
        "<label><input type='radio' name='mode' value='Manual' " + String(mode=="Manual"?"checked":"") + "> Manual</label>"
        "<label><input type='radio' name='mode' value='ML' " + String(mode=="ML"?"checked":"") + "> ML</label>"
      "</div>"
      "<div class='row seg'>"
        "<button class='btn' id='leftBtn'>Left</button>"
        "<button class='btn' id='centerBtn'>Center</button>"
        "<button class='btn' id='rightBtn'>Right</button>"
      "</div>"
      "<div class='row'>"
        "<div>Servo: <span id='servoPos'>" + String(posStr) + "</span> | Angle: <span id='angle'>" + String(angle) + "</span>&deg;</div>"
      "</div>"
      "<div class='row'><div>ML score: <span id='score'>--</span></div></div>"
    "</div>"
    "<script>"
      "function setMode(m){fetch('/set_mode?mode='+m)}"
      "function setServo(p){fetch('/set_servo?pos='+p)}"
      "document.querySelectorAll('input[name=mode]').forEach(r=>{r.addEventListener('change',e=>setMode(e.target.value));});"
      "document.getElementById('leftBtn').onclick=()=>setServo('left');"
      "document.getElementById('centerBtn').onclick=()=>setServo('center');"
      "document.getElementById('rightBtn').onclick=()=>setServo('right');"
      "setInterval(()=>{fetch('/status').then(r=>r.json()).then(s=>{"
        "document.getElementById('temp').innerText=s.temp;"
        "document.getElementById('hum').innerText=s.hum;"
        "document.getElementById('servoPos').innerText=s.servo;"
        "document.getElementById('angle').innerText=s.angle;"
        "document.getElementById('score').innerText=(s.score!==null)?s.score.toFixed(3):'--';"
        "document.querySelectorAll('input[name=mode]').forEach(r=>{r.checked=(r.value===s.mode)});"
      "});},1000);"
    "</script>"
    "</body></html>");
}

String settingsPage() {
  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Settings</title>
      <style>
        body { font-family: Arial; text-align:center; margin:0;}
        .container { margin:20px auto; max-width:350px;background:#f9f9f9;border-radius:10px;box-shadow:0 2px 10px #ccc;padding:20px;}
        input[type=text], input[type=password]{width:90%;padding:10px;}
        button { padding:10px 15px; margin:10px; font-size:18px;}
      </style>
    </head>
    <body>
      <div class='container'>
        <h2>Wi-Fi Settings</h2>
        <form id="wifiForm">
          <input name="ssid" id="ssid" placeholder="SSID" required><br>
          <input name="password" id="pass" type="password" placeholder="Password" required><br><br>
          <button type="submit">Connect</button>
          <button type="button" onclick="window.location='/'">Back</button>
        </form>
        <div id="msg"></div>
      </div>
      <script>
        document.getElementById('wifiForm').onsubmit = function(e){
          e.preventDefault();
          let ssid = document.getElementById('ssid').value;
          let pass = document.getElementById('pass').value;
          fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))
            .then(r=>r.text())
            .then(msg=>{
              document.getElementById('msg').innerText=msg;
            });
        };
      </script>
    </body></html>
  )rawliteral";
}

// ========== Handlers ==========
void handleRoot() { server.send(200, "text/html", mainPage()); }

// legacy LED toggle removed from UI; keep endpoint no-op or remove
void handleToggle() { server.send(200, "application/json", "{}"); }

void handleSensors() {
  float t = get_latest_temperature();
  float h = get_latest_humidity();
  if (isnan(t)) t = 0.0f; if (isnan(h)) h = 0.0f;
  String json = "{\"temp\":"+String(t)+",\"hum\":"+String(h)+"}";
  server.send(200, "application/json", json);
}

static String posToStr(ServoPos p){
  switch(p){
    case SERVO_LEFT: return "Left";
    case SERVO_CENTER: return "Center";
    default: return "Right";
  }
}

void handleSetMode(){
  String m = server.arg("mode");
  if (m == "ML") servo_set_mode(MODE_ML);
  else servo_set_mode(MODE_MANUAL);
  server.send(200, "text/plain", "OK");
}

void handleSetServo(){
  String p = server.arg("pos");
  if (p == "left") servo_set_manual(SERVO_LEFT);
  else if (p == "center") servo_set_manual(SERVO_CENTER);
  else if (p == "right") servo_set_manual(SERVO_RIGHT);
  server.send(200, "text/plain", "OK");
}

void handleStatus(){
  float t = get_latest_temperature(); if (isnan(t)) t = 0.0f;
  float h = get_latest_humidity(); if (isnan(h)) h = 0.0f;
  float score = get_latest_ml_score();
  String mode = (servo_get_mode()==MODE_ML)?"ML":"Manual";
  ServoPos p = servo_get_position();
  int angle = servo_get_angle();
  String json = "{\"temp\":"+String(t)+",\"hum\":"+String(h)+",\"mode\":\""+mode+"\",\"servo\":\""+posToStr(p)+"\",\"angle\":"+String(angle)+",\"score\":";
  if (isnan(score)) json += "null"; else json += String(score, 3);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSettings() { server.send(200, "text/html", settingsPage()); }

void handleConnect() {
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  server.send(200, "text/plain", "Connecting....");
  connecting = true;
  connectToWiFi();
  connect_start_ms = millis();
}

// ========== WiFi ==========
void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle); // legacy
  server.on("/sensors", HTTP_GET, handleSensors); // legacy
  server.on("/set_mode", HTTP_GET, handleSetMode);
  server.on("/set_servo", HTTP_GET, handleSetServo);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.begin();
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
}

void connectToWiFi() {
  // Keep AP up while attempting STA so the settings page/connection doesn't drop
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
}

// ========== Main task ==========
void main_server_task(void *pvParameters){
  pinMode(BOOT_PIN, INPUT_PULLUP);

  startAP();
  setupServer();

  while(1){
    server.handleClient();

    // Nếu nhấn BOOT thì về AP mode
    if (digitalRead(BOOT_PIN) == LOW) {
      vTaskDelay(100);
      if (digitalRead(BOOT_PIN) == LOW) {
        if (!isAPMode) {
          startAP();
          setupServer();
        }
      }
    }

    // Nếu đang connect STA
    if (connecting) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        // Now that STA is up, turn off AP to avoid two networks (optional)
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        isAPMode = false;
        connecting = false;
      } else if (millis() - connect_start_ms > 10000) { // timeout 10s
        Serial.println("WiFi connect failed! Back to AP.");
        startAP();
        setupServer();
        connecting = false;
      }
    }

    vTaskDelay(20); // avoid watchdog reset
  }
}
