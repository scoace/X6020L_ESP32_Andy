#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ModbusRTU.h>

// WLAN-ZugangsdatenIOT
const char* ssid = "IOT";
const char* password = "";

// Modbus-Konfiguration
ModbusRTU mb;
#define TXD2 17
#define RXD2 16
#define SLAVE_ID 1

// Globale Variablen
float setVoltage = 0.0;
float setCurrent = 0.0;
float outputVoltage = 0.0;
float outputCurrent = 0.0;
float inputVoltage = 0.0;
bool powerState = false;
uint16_t registers[14];

// Webserver initialisieren
AsyncWebServer server(80);

// Hilfsfunktionen
void updateRegisters() {
  if (mb.readHreg(SLAVE_ID, 0x0000, registers ,14)) {

    setVoltage = mb.Hreg(0) / 100.0;
    setCurrent = mb.Hreg(1) / 100.0;
    outputVoltage = mb.Hreg(2) / 100.0;
    outputCurrent = mb.Hreg(3) / 100.0;
    inputVoltage = mb.Hreg(5) / 100.0;
    powerState = (mb.Hreg(0x12) & 1);
  }
}

void setPowerState(bool state) {
  mb.writeHreg(SLAVE_ID, 0x12, state ? 1 : 0);
}

void setVoltageAndCurrent(float voltage, float current) {
  mb.writeHreg(SLAVE_ID, 0x0000, voltage * 100);
  mb.writeHreg(SLAVE_ID, 0x0001, current * 100);
}

// Setup-Funktion
void setup() {
  Serial.begin(115200);

  // Modbus initialisieren
  mb.begin(&Serial2, RXD2, TXD2);
  mb.master();

  // WLAN verbinden
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Verbinde mit WLAN...");
  }
  Serial.println("WLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Routen für den Webserver definieren
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<h1>ESP32 Modbus Control</h1>";
    html += "<p>Set Voltage: " + String(setVoltage, 2) + " V</p>";
    html += "<p>Set Current: " + String(setCurrent, 2) + " A</p>";
    html += "<p>Output Voltage: " + String(outputVoltage, 2) + " V</p>";
    html += "<p>Output Current: " + String(outputCurrent, 2) + " A</p>";
    html += "<p>Input Voltage: " + String(inputVoltage, 2) + " V</p>";
    html += "<p>Power State: " + String(powerState ? "On" : "Off") + "</p>";
    html += "<a href=\"/on\">Turn On</a><br>";
    html += "<a href=\"/off\">Turn Off</a><br>";
    html += "<form action=\"/set\" method=\"GET\">";
    html += "Voltage: <input type=\"text\" name=\"voltage\"><br>";
    html += "Current: <input type=\"text\" name=\"current\"><br>";
    html += "<input type=\"submit\" value=\"Set Values\">";
    html += "</form>";
    request->send(200, "text/html", html);
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    setPowerState(true);
    request->send(200, "text/html", "<p>Power turned ON. <a href=\"/\">Back</a></p>");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    setPowerState(false);
    request->send(200, "text/html", "<p>Power turned OFF. <a href=\"/\">Back</a></p>");
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("voltage") && request->hasParam("current")) {
      float voltage = request->getParam("voltage")->value().toFloat();
      float current = request->getParam("current")->value().toFloat();
      setVoltageAndCurrent(voltage, current);
      request->send(200, "text/html", "<p>Values updated. <a href=\"/\">Back</a></p>");
    } else {
      request->send(400, "text/html", "<p>Missing parameters. <a href=\"/\">Back</a></p>");
    }
  });

  // Webserver starten
  server.begin();
}

// Loop-Funktion
void loop() {
  mb.task();
  delay(100);
  updateRegisters();
}
