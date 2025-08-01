#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>


// WiFi credentials
const char* ssid = "Getnet";
const char* password = "12345ghk";

// Pins
#define BUZZER_PIN 26
#define PIR_PIN 33
#define DHT_PIN 27
#define LED_R_PIN 19
#define LED_G_PIN 21
#define LED_B_PIN 22

// PWM settings (8-bit resolution, 0-255)
#define PWM_RESOLUTION 8
#define PWM_FREQUENCY 1000  // 1KHz (adjust if needed)

// DHT Sensor
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Web server
WebServer server(80);

// System variables
float temperature = 0;
float humidity = 0;
bool pirState = false;
bool ledAutoMode = true;
int ledR = 0;
int ledG = 0;
int ledB = 0;
int buzzerVolume = 50; // 0-100
bool systemArmed = true;
String alertMessage = "";

// User credentials
const String adminUser = "TitusVybes";
const String adminPass = "12345Ghk";
bool isAuthenticated = false;

// Function prototypes
void handleLoginPage();
void handleLogin();
void handleLogout();
void handleDashboard();
void handleSettings();
void handleNotifications();
void handleContact();
void handleFAQ();
void handleSensorData();
void handleUpdateSettings();
void handleControlLed();
void readSensors();
void updateOutputs();
void checkAlerts();
String getHeader();
String getSidebar();
String getFooter();

void setup() {
  Serial.begin(115200);

 // Initialize PWM (using analogWrite-style)
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

 

  // Initialize PIR sensor
  pinMode(PIR_PIN, INPUT);

  // Initialize DHT sensor
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Route handlers
  server.on("/", handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);
  server.on("/dashboard", handleDashboard);
  server.on("/settings", handleSettings);
  server.on("/notifications", handleNotifications);
  server.on("/contact", handleContact);
  server.on("/faq", handleFAQ);
  server.on("/api/sensorData", handleSensorData);
  server.on("/api/updateSettings", HTTP_POST, handleUpdateSettings);
  server.on("/api/controlLed", HTTP_POST, handleControlLed);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 2000) { // Read sensors every 2 sec
    readSensors();
    lastSensorRead = millis();
    checkAlerts();
  }
  
  updateOutputs();
}

void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  pirState = digitalRead(PIR_PIN) == HIGH;
  
  if (ledAutoMode) {
    if (pirState && systemArmed) {
      ledR = 255; // Red if motion detected
      ledG = 0;
      ledB = 0;
    } else if (!systemArmed) {
      ledR = 0;
      ledG = 0;
      ledB = 255; // Blue if disarmed
    } else {
      ledR = 0;
      ledG = 255; // Green if normal
      ledB = 0;
    }
  }
}

void updateOutputs() {
  // Update LEDs (using analogWrite)
  analogWrite(LED_R_PIN, ledR);
  analogWrite(LED_G_PIN, ledG);
  analogWrite(LED_B_PIN, ledB);

  // Handle buzzer alerts
  if (alertMessage != "" && systemArmed) {
    int volume = map(buzzerVolume, 0, 100, 0, 255);
    analogWrite(BUZZER_PIN, volume);
    delay(100);
    analogWrite(BUZZER_PIN, 0);
    delay(100);
  } else {
    analogWrite(BUZZER_PIN, 0);
  }
}

void checkAlerts() {
  static String lastAlert = "";
  
  if (!systemArmed) {
    alertMessage = "";
    return;
  }
  
  if (pirState && temperature > 30) {
    alertMessage = "ALERT: Intruder detected and high temperature!";
  } else if (pirState) {
    alertMessage = "ALERT: Intruder detected!";
  } else if (temperature > 30) {
    alertMessage = "WARNING: High temperature detected!";
  } else {
    alertMessage = "";
  }
  
  if (alertMessage != "" && alertMessage != lastAlert) {
    Serial.println(alertMessage);
    lastAlert = alertMessage;
  }
}

// ================== WEB INTERFACE FUNCTIONS ==================

String getHeader() {
  return R"=====(
    <header style="background-color: #2c3e50; color: white; padding: 1rem; display: flex; justify-content: space-between; align-items: center;">
      <h1>Smart Security System</h1>
      <nav>
        <a href="/dashboard" style="color: white; margin: 0 10px; text-decoration: none;">Home</a>
        <a href="/dashboard" style="color: white; margin: 0 10px; text-decoration: none;">Dashboard</a>
        <a href="/settings" style="color: white; margin: 0 10px; text-decoration: none;">Settings</a>
        <a href="/logout" style="color: white; margin: 0 10px; text-decoration: none;">Logout</a>
      </nav>
    </header>
  )=====";
}

String getSidebar() {
  return R"=====(
    <div id="sidebar" style="width: 200px; background-color: #34495e; color: white; padding: 1rem; height: calc(100vh - 150px); float: left;">
      <ul style="list-style-type: none; padding: 0;">
        <li style="margin: 15px 0;"><a href="/dashboard" style="color: white; text-decoration: none;">Dashboard</a></li>
        <li style="margin: 15px 0;"><a href="/notifications" style="color: white; text-decoration: none;">Notifications</a></li>
        <li style="margin: 15px 0;"><a href="/faq" style="color: white; text-decoration: none;">FAQs & Help</a></li>
        <li style="margin: 15px 0;"><a href="/contact" style="color: white; text-decoration: none;">Contact</a></li>
        <li style="margin: 15px 0;"><a href="/logout" style="color: white; text-decoration: none;">Logout</a></li>
      </ul>
    </div>
  )=====";
}

String getFooter() {
  return R"=====(
    <footer style="background-color: #2c3e50; color: white; text-align: center; padding: 1rem; position: fixed; bottom: 0; width: 100%;">
      &copy;2025 Titus Smart Security System - Embedded Systems Course
    </footer>
  )=====";
}

void handleLoginPage() {
  if (isAuthenticated) {
    server.sendHeader("Location", "/dashboard");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Smart Security System - Login</title>
      <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }
        .login-container { background: white; padding: 2rem; border-radius: 5px; box-shadow: 0 0 10px rgba(0,0,0,0.1); width: 300px; }
        h2 { text-align: center; color: #2c3e50; }
        input { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { background-color: #2c3e50; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; width: 100%; }
        button:hover { background-color: #1a252f; }
      </style>
    </head>
    <body>
      <div class="login-container">
        <h2>Login</h2>
        <form action="/login" method="POST">
          <input type="text" name="username" placeholder="Username" required>
          <input type="password" name="password" placeholder="Password" required>
          <button type="submit">Login</button>
        </form>
      </div>
    </body>
    </html>
  )=====";
  
  server.send(200, "text/html", html);
}

void handleLogin() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String username = server.arg("username");
  String password = server.arg("password");

  if (username == adminUser && password == adminPass) {
    isAuthenticated = true;
    server.sendHeader("Location", "/dashboard");
    server.send(302, "text/plain", "");
  } else {
    String html = R"=====(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Login Failed</title>
        <style>
          body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }
          .message { background: white; padding: 2rem; border-radius: 5px; box-shadow: 0 0 10px rgba(0,0,0,0.1); text-align: center; }
          a { color: #2c3e50; text-decoration: none; }
        </style>
      </head>
      <body>
        <div class="message">
          <h2>Invalid credentials</h2>
          <p><a href="/">Try again</a></p>
        </div>
      </body>
      </html>
    )=====";
    server.send(200, "text/html", html);
  }
}

void handleLogout() {
  isAuthenticated = false;
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleDashboard() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Dashboard - Smart Security System</title>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #ecf0f1; }
        .content { margin-left: 220px; padding: 20px; }
        .card { background: white; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }
        .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; }
        .sensor-value { font-size: 24px; font-weight: bold; color: #2c3e50; }
        .alert { color: #e74c3c; font-weight: bold; }
        .controls { display: flex; flex-direction: column; gap: 15px; }
        button { padding: 10px; border: none; border-radius: 4px; cursor: pointer; }
        .btn-primary { background-color: #2c3e50; color: white; }
        .btn-danger { background-color: #e74c3c; color: white; }
        .btn-success { background-color: #27ae60; color: white; }
        .rgb-controls { display: flex; gap: 10px; align-items: center; }
        .color-preview { width: 30px; height: 30px; border-radius: 50%; border: 1px solid #ddd; }
      </style>
      <script>
        function updateSensorData() {
          fetch('/api/sensorData')
            .then(response => response.json())
            .then(data => {
              document.getElementById('temp-value').textContent = data.temperature + '°C';
              document.getElementById('humidity-value').textContent = data.humidity + '%';
              document.getElementById('pir-value').textContent = data.pirState ? 'Motion Detected' : 'No Motion';
              document.getElementById('pir-value').className = data.pirState ? 'sensor-value alert' : 'sensor-value';
              document.getElementById('alert-message').textContent = data.alertMessage;
              document.getElementById('alert-message').className = data.alertMessage ? 'alert' : '';
              document.getElementById('system-status').textContent = data.systemArmed ? 'Armed' : 'Disarmed';
              document.getElementById('system-status').className = data.systemArmed ? 'sensor-value' : 'sensor-value alert';
            });
        }
        
        function toggleSystem() {
          fetch('/api/updateSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'systemArmed=' + !document.getElementById('system-status').textContent.includes('Armed')
          }).then(() => updateSensorData());
        }
        
        function updateBuzzerVolume() {
          const volume = document.getElementById('buzzer-volume').value;
          fetch('/api/updateSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'buzzerVolume=' + volume
          });
        }
        
        function updateLedColor() {
          const r = document.getElementById('led-r').value;
          const g = document.getElementById('led-g').value;
          const b = document.getElementById('led-b').value;
          document.getElementById('color-preview').style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
          
          fetch('/api/controlLed', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `r=${r}&g=${g}&b=${b}`
          });
        }
        
        function toggleLedMode() {
          const autoMode = document.getElementById('led-auto').checked;
          fetch('/api/updateSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'ledAutoMode=' + autoMode
          });
          
          document.getElementById('manual-controls').style.display = autoMode ? 'none' : 'block';
        }
        
        setInterval(updateSensorData, 2000);
        window.onload = updateSensorData;
      </script>
    </head>
    <body>
  )=====";
  
  html += getHeader();
  html += getSidebar();
  
  html += R"=====(
    <div class="content">
      <h2>System Dashboard</h2>
      
      <div class="card">
        <h3>Sensor Status</h3>
        <div class="sensor-grid">
          <div>
            <p>Temperature</p>
            <p id="temp-value" class="sensor-value">0°C</p>
          </div>
          <div>
            <p>Humidity</p>
            <p id="humidity-value" class="sensor-value">0%</p>
          </div>
          <div>
            <p>Motion Detection</p>
            <p id="pir-value" class="sensor-value">No Motion</p>
          </div>
          <div>
            <p>System Status</p>
            <p id="system-status" class="sensor-value">Armed</p>
          </div>
        </div>
      </div>
      
      <div class="card">
        <h3>Alerts</h3>
        <p id="alert-message"></p>
      </div>
      
      <div class="card">
        <h3>System Controls</h3>
          <button onclick="toggleSystem();" class="btn-primary">Toggle System Armed/Disarmed</button>
          
          <div>
            <label>Buzzer Volume: </label>
            <input type="range" id="buzzer-volume" min="0" max="100" value=")=====" + String(buzzerVolume) + R"=====(" onchange="updateBuzzerVolume()">
            <span>)=====" + String(buzzerVolume) + R"=====(%</span>
          </div>
          
          <div>
            <label>
              <input type="checkbox" id="led-auto" onchange="toggleLedMode();" )=====" + (ledAutoMode ? "checked" : "") + R"=====(> Automatic LED Control
            </label>
          </div>
          
          <div id="manual-controls" style="display: )=====" + (ledAutoMode ? "none" : "block") + R"=====(;">
            <h4>Manual RGB Control</h4>
            <div class="rgb-controls">
              <div>
                <label>Red</label>
                <input type="range" id="led-r" min="0" max="255" value=")=====" + String(ledR) + R"=====(" oninput="updateLedColor();">
              </div>
              <div>
                <label>Green</label>
                <input type="range" id="led-g" min="0" max="255" value=")=====" + String(ledG) + R"=====(" oninput="updateLedColor();">
              </div>
              <div>
                <label>Blue</label>
                <input type="range" id="led-b" min="0" max="255" value=")=====" + String(ledB) + R"=====(" oninput="updateLedColor();">
              </div>
              <div id="color-preview" class="color-preview" style="background-color: rgb()=====" + String(ledR) + "," + String(ledG) + "," + String(ledB) + R"=====(;)"></div>
            </div>
          </div>
        </div>
      </div>
    </div>
  )=====";
  
  html += getFooter();
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSettings() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Settings - Smart Security System</title>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #ecf0f1; }
        .content { margin-left: 220px; padding: 20px; }
        .card { background: white; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { background-color: #2c3e50; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; }
      </style>
    </head>
    <body>
  )=====";
  
  html += getHeader();
  html += getSidebar();
  
  html += R"=====(
    <div class="content">
      <h2>System Settings</h2>
      
      <div class="card">
        <h3>WiFi Settings</h3>
        <form id="wifi-form">
          <div class="form-group">
            <label for="ssid">SSID</label>
            <input type="text" id="ssid" name="ssid" placeholder="WiFi Network Name">
          </div>
          <div class="form-group">
            <label for="password">Password</label>
            <input type="password" id="password" name="password" placeholder="WiFi Password">
          </div>
          <button type="button" onclick="alert('In a real system, this would save WiFi settings');">Save WiFi Settings</button>
        </form>
      </div>
      
      <div class="card">
        <h3>Notification Settings</h3>
        <form id="notification-form">
          <div class="form-group">
            <label for="email">Email for Alerts</label>
            <input type="email" id="email" name="email" placeholder="your&#64;email.com">
          </div>
          <div class="form-group">
            <label for="phone">Phone for SMS Alerts</label>
            <input type="tel" id="phone" name="phone" placeholder="+1234567890">
          </div>
          <div class="form-group">
            <label>
              <input type="checkbox" id="email-alerts" name="email-alerts" checked> Enable Email Alerts
            </label>
          </div>
          <div class="form-group">
            <label>
              <input type="checkbox" id="sms-alerts" name="sms-alerts"> Enable SMS Alerts
            </label>
          </div>
          <button type="button" onclick="alert('In a real system, this would save notification settings');">Save Notification Settings</button>
        </form>
      </div>
    </div>
  )=====";
  
  html += getFooter();
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleNotifications() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Notifications - Smart Security System</title>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #ecf0f1; }
        .content { margin-left: 220px; padding: 20px; }
        .card { background: white; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }
        .notification { padding: 10px; border-bottom: 1px solid #eee; }
        .notification.unread { background-color: #f9f9f9; }
        .notification-time { color: #7f8c8d; font-size: 0.8em; }
      </style>
    </head>
    <body>
  )=====";
  
  html += getHeader();
  html += getSidebar();
  
  html += R"=====(
    <div class="content">
      <h2>Notifications</h2>
      
      <div class="card">
        <h3>Recent Alerts</h3>
        <div class="notification unread">
          <p>System armed at 10:30 AM</p>
          <p class="notification-time">Today, 10:30 AM</p>
        </div>
        <div class="notification">
          <p>Motion detected in living room</p>
          <p class="notification-time">Yesterday, 8:45 PM</p>
        </div>
        <div class="notification">
          <p>High temperature detected (32°C)</p>
          <p class="notification-time">Yesterday, 3:20 PM</p>
        </div>
      </div>
    </div>
  )=====";
  
  html += getFooter();
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleContact() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Contact - Smart Security System</title>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #ecf0f1; }
        .content { margin-left: 220px; padding: 20px; }
        .card { background: white; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, textarea { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { background-color: #2c3e50; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; }
      </style>
    </head>
    <body>
  )=====";
  
  html += getHeader();
  html += getSidebar();
  
  html += R"=====(
    <div class="content">
      <h2>Contact Support</h2>
      
      <div class="card">
        <form id="contact-form">
          <div class="form-group">
            <label for="name">Your Name</label>
            <input type="text" id="name" name="name" placeholder="Your Name">
          </div>
          <div class="form-group">
            <label for="email">Your Email</label>
            <input type="email" id="email" name="email" placeholder="your&#64;email.com">
          </div>
          <div class="form-group">
            <label for="subject">Subject</label>
            <input type="text" id="subject" name="subject" placeholder="Subject">
          </div>
          <div class="form-group">
            <label for="message">Message</label>
            <textarea id="message" name="message" rows="5" placeholder="Your message"></textarea>
          </div>
          <button type="button" onclick="alert('In a real system, this would send your message');">Send Message</button>
        </form>
      </div>
      
      <div class="card">
        <h3>Support Information</h3>
        <p><strong>Email:</strong> support&#64;smartsecurity.com</p>
        <p><strong>Phone:</strong> +1 (555) 123-4567</p>
        <p><strong>Hours:</strong> Monday-Friday, 9AM-5PM</p>
      </div>
    </div>
  )=====";
  
  html += getFooter();
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleFAQ() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }

  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <title>FAQs & Help - Smart Security System</title>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #ecf0f1; }
        .content { margin-left: 220px; padding: 20px; }
        .card { background: white; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }
        .faq-item { margin-bottom: 15px; }
        .faq-question { font-weight: bold; color: #2c3e50; cursor: pointer; }
        .faq-answer { margin-top: 5px; padding: 10px; background-color: #f9f9f9; border-radius: 4px; }
      </style>
      <script>
        function toggleAnswer(id) {
          const answer = document.getElementById('answer-' + id);
          answer.style.display = answer.style.display === 'none' ? 'block' : 'none';
        }
      </script>
    </head>
    <body>
  )=====";

  html += getHeader();
  html += getSidebar();

  html += R"=====(
    <div class="content">
      <h2>Frequently Asked Questions</h2>
      <div class="card">
        <div class="faq-item">
          <div class="faq-question" onclick="toggleAnswer('1')">How do I arm/disarm the system?</div>
          <div id="answer-1" class="faq-answer" style="display: none;">
            <p>You can arm or disarm the system from the Dashboard page by clicking the "Toggle System Armed/Disarmed" button. When armed, the system will monitor for motion and other alerts.</p>
          </div>
        </div>
        <div class="faq-item">
          <div class="faq-question" onclick="toggleAnswer('2')">How do I change the buzzer volume?</div>
          <div id="answer-2" class="faq-answer" style="display: none;">
            <p>On the Dashboard page, use the "Buzzer Volume" slider to adjust the volume of the alarm buzzer. The change takes effect immediately.</p>
          </div>
        </div>
        <div class="faq-item">
          <div class="faq-question" onclick="toggleAnswer('3')">How does the automatic LED mode work?</div>
          <div id="answer-3" class="faq-answer" style="display: none;">
            <p>In automatic mode, the LED will change color based on system status: Green when normal, Red when motion is detected, and Blue when the system is disarmed. You can switch to manual mode from the Dashboard.</p>
          </div>
        </div>
        <div class="faq-item">
          <div class="faq-question" onclick="toggleAnswer('4')">How do I receive alerts when I\'m away?</div>
          <div id="answer-4" class="faq-answer" style="display: none;">
            <p>Go to the Settings page and enter your email and/or phone number in the Notification Settings section. Make sure to enable the types of alerts you want to receive.</p>
          </div>
        </div>
        <div class="faq-item">
          <div class="faq-question" onclick="toggleAnswer('5')">What should I do if I forget my password?</div>
          <div id="answer-5" class="faq-answer" style="display: none;">
            <p>Contact our support team at support&#64;smartsecurity.com or call +1 (555) 123-4567 for assistance with password recovery.</p>
          </div>
        </div>
      </div>
    </div>
  )=====";

  html += getFooter();
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSensorData() {
  if (!isAuthenticated) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  
  DynamicJsonDocument doc(256);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pirState"] = pirState;
  doc["alertMessage"] = alertMessage;
  doc["systemArmed"] = systemArmed;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleUpdateSettings() {
  if (!isAuthenticated) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  if (server.hasArg("buzzerVolume")) {
    buzzerVolume = server.arg("buzzerVolume").toInt();
  }

  if (server.hasArg("ledAutoMode")) {
    ledAutoMode = (server.arg("ledAutoMode") == "true");
  }

  if (server.hasArg("systemArmed")) {
    systemArmed = (server.arg("systemArmed") == "true");
  }

  server.send(200, "text/plain", "Settings updated");
}

void handleControlLed() {
  if (!isAuthenticated) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  if (!ledAutoMode) {
    if (server.hasArg("r")) {
      ledR = server.arg("r").toInt();
    }
    if (server.hasArg("g")) {
      ledG = server.arg("g").toInt();
    }
    if (server.hasArg("b")) {
      ledB = server.arg("b").toInt();
    }
    server.send(200, "text/plain", "LED color updated");
  } else {
    server.send(403, "text/plain", "LED manual control disabled in auto mode");
  }
}