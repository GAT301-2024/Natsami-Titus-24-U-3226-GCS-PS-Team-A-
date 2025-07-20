#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Wi-Fi Configuration
const char* ssid = "Getnet";
const char* password = "12345ghk";

// LED Control Pins (Based on schematic)
const int LED1_CTRL_PIN = 18;  // GPIO18 → Q1 base (LED1)
const int LED2_CTRL_PIN = 19;  // GPIO19 → Q2 base (LED2)
const int LED3_CTRL_PIN = 21;  // GPIO21 → Q3 base (LED3)

// LDR Pin
const int LDR_PIN = 34; // GPIO34 for analog reading

// Automatic Control Settings
const int NIGHT_THRESHOLD = 800; // Calibrate this value
const long AUTO_CHECK_INTERVAL = 10000; // 10 seconds

// Web Server
AsyncWebServer server(80);

// System State
bool led1State = false;
bool led2State = false;
bool led3State = false;
bool autoModeEnabled = false;
unsigned long lastAutoCheckMillis = 0;

// Set LED state helper
void setLED(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nNatsami Titus Intelligent Lighting System - Team A");

  // Initialize pins
  pinMode(LED1_CTRL_PIN, OUTPUT);
  pinMode(LED2_CTRL_PIN, OUTPUT);
  pinMode(LED3_CTRL_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // Initialize LEDs to OFF
  setLED(LED1_CTRL_PIN, LOW);
  setLED(LED2_CTRL_PIN, LOW);
  setLED(LED3_CTRL_PIN, LOW);

  // Start WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Route handlers
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", getDashboardHtml());
  });

  server.on("/led1/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led1State = !led1State;
    setLED(LED1_CTRL_PIN, led1State);
    request->send(200, "text/plain", led1State ? "LED1_ON" : "LED1_OFF");
  });

  server.on("/led2/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led2State = !led2State;
    setLED(LED2_CTRL_PIN, led2State);
    request->send(200, "text/plain", led2State ? "LED2_ON" : "LED2_OFF");
  });

  server.on("/led3/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led3State = !led3State;
    setLED(LED3_CTRL_PIN, led3State);
    request->send(200, "text/plain", led3State ? "LED3_ON" : "LED3_OFF");
  });

  server.on("/automode/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoModeEnabled = !autoModeEnabled;
    String jsonResponse = "{ \"autoModeEnabled\": " + String(autoModeEnabled ? "true" : "false") + " }";
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    int ldrValue = analogRead(LDR_PIN);
    String jsonResponse = "{";
    jsonResponse += "\"led1\":" + String(led1State ? "true" : "false") + ",";
    jsonResponse += "\"led2\":" + String(led2State ? "true" : "false") + ",";
    jsonResponse += "\"led3\":" + String(led3State ? "true" : "false") + ",";
    jsonResponse += "\"autoModeEnabled\":" + String(autoModeEnabled ? "true" : "false") + ",";
    jsonResponse += "\"ldrValue\":" + String(ldrValue) + ",";
    jsonResponse += "\"system\":\"NatsamiTitus Lighting System\"";
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if (autoModeEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAutoCheckMillis >= AUTO_CHECK_INTERVAL) {
      lastAutoCheckMillis = currentMillis;
      int ldrValue = analogRead(LDR_PIN);
      
      if (ldrValue < NIGHT_THRESHOLD) {
        if (!led1State) { led1State = true; setLED(LED1_CTRL_PIN, HIGH); }
        if (!led2State) { led2State = true; setLED(LED2_CTRL_PIN, HIGH); }
        if (!led3State) { led3State = true; setLED(LED3_CTRL_PIN, HIGH); }
      } else {
        if (led1State) { led1State = false; setLED(LED1_CTRL_PIN, LOW); }
        if (led2State) { led2State = false; setLED(LED2_CTRL_PIN, LOW); }
        if (led3State) { led3State = false; setLED(LED3_CTRL_PIN, LOW); }
      }
    }
  }
}