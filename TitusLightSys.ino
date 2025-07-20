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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NatsamiTitus Systems - Smart Lighting</title>
    <link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@400;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #4a6fa5;
            --secondary: #166088;
            --accent: #4fc3f7;
            --dark: #0a1128;
            --light: #f8f9fa;
            --success: #28a745;
            --danger: #dc3545;
            --warning: #ffc107;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Montserrat', sans-serif;
            background: linear-gradient(rgba(0,0,0,0.7), rgba(0,0,0,0.7)), 
                        url('https://images.unsplash.com/photo-1551269901-5c5e14c25df7?ixlib=rb-1.2.1&auto=format&fit=crop&w=1350&q=80');
            background-size: cover;
            background-position: center;
            color: var(--light);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }
        
        .container {
            max-width: 800px;
            margin: 2rem auto;
            padding: 2rem;
            background: rgba(10, 17, 40, 0.85);
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            backdrop-filter: blur(5px);
        }
        
        header {
            text-align: center;
            margin-bottom: 2rem;
            padding-bottom: 1rem;
            border-bottom: 2px solid var(--accent);
        }
        
        h1 {
            font-size: 2.5rem;
            margin-bottom: 0.5rem;
            color: var(--accent);
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .subtitle {
            font-size: 1rem;
            color: var(--light);
            opacity: 0.8;
        }
        
        .control-panel {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }
        
        .control-card {
            background: rgba(74, 111, 165, 0.2);
            border-radius: 10px;
            padding: 1.5rem;
            border: 1px solid rgba(74, 111, 165, 0.3);
            transition: all 0.3s ease;
        }
        
        .control-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 10px 20px rgba(0,0,0,0.2);
            border-color: var(--accent);
        }
        
        .control-card h2 {
            font-size: 1.2rem;
            margin-bottom: 1rem;
            color: var(--accent);
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        
        .status-indicator {
            display: inline-block;
            width: 15px;
            height: 15px;
            border-radius: 50%;
            background-color: var(--danger);
            margin-left: 0.5rem;
        }
        
        .status-indicator.on {
            background-color: var(--success);
            box-shadow: 0 0 10px var(--success);
        }
        
        .btn {
            display: block;
            width: 100%;
            padding: 0.8rem;
            border: none;
            border-radius: 5px;
            background: var(--primary);
            color: white;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-top: 1rem;
        }
        
        .btn:hover {
            background: var(--secondary);
            transform: translateY(-2px);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn.on {
            background: var(--success);
        }
        
        .sensor-data {
            background: rgba(22, 96, 136, 0.3);
            padding: 1.5rem;
            border-radius: 10px;
            margin-bottom: 2rem;
        }
        
        .sensor-data h2 {
            margin-bottom: 1rem;
            color: var(--accent);
        }
        
        .data-display {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .data-value {
            font-size: 1.5rem;
            font-weight: 600;
            color: var(--accent);
        }
        
        footer {
            text-align: center;
            padding: 1.5rem;
            margin-top: auto;
            background: rgba(10, 17, 40, 0.8);
        }
        
        .footer-text {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        
        .team-name {
            color: var(--accent);
            font-weight: 600;
        }
        
        @media (max-width: 768px) {
            .container {
                margin: 1rem;
                padding: 1.5rem;
            }
            
            h1 {
                font-size: 2rem;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>NatsamiTitus Systems</h1>
            <p class="subtitle">Intelligent Lighting Control Panel</p>
        </header>
        
        <div class="sensor-data">
            <h2>Environment Sensor</h2>
            <div class="data-display">
                <div>
                    <p>Light Level</p>
                    <p class="data-value" id="ldrValue">---</p>
                </div>
                <button id="autoModeButton" class="btn">AUTO MODE: OFF</button>
                <span id="autoModeStatus" class="status-indicator"></span>
            </div>
        </div>
        
        <div class="control-panel">
            <div class="control-card">
                <h2>LED 1 <span id="led1Status" class="status-indicator"></span></h2>
                <button id="led1Button" class="btn">Toggle LED 1</button>
            </div>
            
            <div class="control-card">
                <h2>LED 2 <span id="led2Status" class="status-indicator"></span></h2>
                <button id="led2Button" class="btn">Toggle LED 2</button>
            </div>
            
            <div class="control-card">
                <h2>LED 3 <span id="led3Status" class="status-indicator"></span></h2>
                <button id="led3Button" class="btn">Toggle LED 3</button>
            </div>
        </div>
    </div>
    
    <footer>
        <p class="footer-text">&copy; 2024 NatsamiTitus Systems | <span class="team-name">Team A</span></p>
        <p class="footer-text">Reg: 24/U/3226/GCS/PS | ESP32 Smart Lighting Controller</p>
    </footer>
    
    <script>
        // DOM Elements
        const led1Button = document.getElementById('led1Button');
        const led2Button = document.getElementById('led2Button');
        const led3Button = document.getElementById('led3Button');
        const autoModeButton = document.getElementById('autoModeButton');
        
        const led1Status = document.getElementById('led1Status');
        const led2Status = document.getElementById('led2Status');
        const led3Status = document.getElementById('led3Status');
        const autoModeStatus = document.getElementById('autoModeStatus');
        
        const ldrValue = document.getElementById('ldrValue');
        
        // Toggle LED functions
        async function toggleLED(ledNum) {
            try {
                const response = await fetch('/led' + ledNum + '/toggle');
                const data = await response.text();
                updateLEDUI(ledNum, data.includes("ON"));
            } catch (error) {
                console.error('Error:', error);
                alert('Failed to communicate with the device');
            }
        }
        
        // Toggle Auto Mode
        async function toggleAutoMode() {
            try {
                const response = await fetch('/automode/toggle');
                const data = await response.json();
                updateAutoModeUI(data.autoModeEnabled);
            } catch (error) {
                console.error('Error:', error);
                alert('Failed to toggle auto mode');
            }
        }
        
        // Update LED UI
        function updateLEDUI(ledNum, state) {
            const button = document.getElementById('led' + ledNum + 'Button');
            const status = document.getElementById('led' + ledNum + 'Status');
            
            if (state) {
                button.classList.add('on');
                button.textContent = 'LED ' + ledNum + ' ON';
                status.classList.add('on');
                status.classList.remove('off');
            } else {
                button.classList.remove('on');
                button.textContent = 'LED ' + ledNum + ' OFF';
                status.classList.add('off');
                status.classList.remove('on');
            }
        }
        
        // Update Auto Mode UI
        function updateAutoModeUI(state) {
            if (state) {
                autoModeButton.classList.add('on');
                autoModeButton.textContent = 'AUTO MODE: ON';
                autoModeStatus.classList.add('on');
                autoModeStatus.classList.remove('off');
            } else {
                autoModeButton.classList.remove('on');
                autoModeButton.textContent = 'AUTO MODE: OFF';
                autoModeStatus.classList.add('off');
                autoModeStatus.classList.remove('on');
            }
        }
        
        // Fetch system status
        async function updateSystemStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                updateLEDUI(1, data.led1);
                updateLEDUI(2, data.led2);
                updateLEDUI(3, data.led3);
                updateAutoModeUI(data.autoModeEnabled);
                ldrValue.textContent = data.ldrValue;
                
                console.log('System status updated:', data);
            } catch (error) {
                console.error('Error fetching status:', error);
            }
        }
        
        // Event listeners
        led1Button.addEventListener('click', () => toggleLED(1));
        led2Button.addEventListener('click', () => toggleLED(2));
        led3Button.addEventListener('click', () => toggleLED(3));
        autoModeButton.addEventListener('click', toggleAutoMode);
        
        // Initial status update
        updateSystemStatus();
        
        // Periodic status updates
        setInterval(updateSystemStatus, 3000);
    </script>
</body>
</html>
)rawliteral";

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
    request->send_P(200, "text/html", index_html);
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
