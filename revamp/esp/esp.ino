/*
 * ========================================
 *  MOODBOT - ESP32 (REVAMPED V2)
 * ========================================
 * Stable WiFi AP, OLED animations, reliable Serial
 * Upload this to ESP32
 * 
 * WIRING (SAME AS BEFORE):
 * OLED Display:
 *   SDA → GPIO 21  |  VCC → 3.3V
 *   SCL → GPIO 22  |  GND → GND
 * 
 * Arduino Serial:
 *   RX2 (GPIO 16) → Arduino TX (Pin 1)
 *   TX2 (GPIO 17) → Arduino RX (Pin 0)
 *   GND → Arduino GND
 * 
 * WiFi Network:
 *   SSID: MoodBot
 *   Password: 12345678
 *   IP: 192.168.4.1
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi AP Settings
const char* ssid = "MoodBot";
const char* password = "12345678";

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Serial to Arduino
#define RXD2 16
#define TXD2 17

WebServer server(80);

// Robot state
String currentMood = "happy";
String currentState = "stopped";
String currentMode = "manual";

// Animation
unsigned long lastAnimUpdate = 0;
int animFrame = 0;
int blinkTimer = 0;
bool isBlinking = false;

// Command rate limiting
unsigned long lastCommandTime = 0;
#define COMMAND_DELAY 100  // ms between commands

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed! Check wiring.");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Show startup message
  showMessage("MoodBot\nStarting...");
  delay(1000);
  
  // Create WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n=================================");
  Serial.println("MoodBot WiFi Ready!");
  Serial.println("=================================");
  Serial.print("Network: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP: ");
  Serial.println(IP);
  Serial.println("=================================\n");
  
  showMessage("WiFi: MoodBot\nPass: 12345678\n\nhttp://\n192.168.4.1");
  delay(3000);
  
  // Setup web routes
  server.on("/", handleRoot);
  server.on("/cmd", HTTP_GET, handleCommand);
  server.on("/mood", HTTP_GET, handleMood);
  server.on("/status", HTTP_GET, handleStatus);
  
  server.begin();
  Serial.println("Web server started!\n");
  
  currentMood = "happy";
}

void loop() {
  server.handleClient();
  
  // Read Arduino responses (non-blocking)
  while (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      parseArduinoMessage(msg);
    }
  }
  
  // Update OLED animation (30 FPS)
  if (millis() - lastAnimUpdate > 33) {
    lastAnimUpdate = millis();
    animFrame++;
    updateDisplay();
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MoodBot Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 15px;
      min-height: 100vh;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background: rgba(255,255,255,0.15);
      backdrop-filter: blur(10px);
      border-radius: 15px;
      padding: 20px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
    }
    h1 {
      text-align: center;
      margin-bottom: 20px;
      font-size: 2em;
    }
    .status {
      background: rgba(255,255,255,0.2);
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 15px;
      text-align: center;
      font-weight: bold;
    }
    .controls {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
      margin: 15px 0;
    }
    button {
      background: rgba(255,255,255,0.2);
      border: 2px solid white;
      color: white;
      padding: 18px;
      font-size: 1.1em;
      border-radius: 8px;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.2s;
    }
    button:active {
      transform: scale(0.95);
      background: rgba(255,255,255,0.3);
    }
    .btn-fwd { grid-column: 2; }
    .btn-left { grid-column: 1; grid-row: 2; }
    .btn-stop { 
      grid-column: 2; 
      grid-row: 2;
      background: rgba(255,0,0,0.3);
    }
    .btn-right { grid-column: 3; grid-row: 2; }
    .btn-back { grid-column: 2; grid-row: 3; }
    .moods {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
      margin: 15px 0;
    }
    .mood-btn { padding: 12px; font-size: 1em; }
    .auto-btn {
      width: 100%;
      background: rgba(0,255,0,0.3);
      margin-top: 10px;
    }
    .info {
      text-align: center;
      margin-top: 15px;
      font-size: 0.9em;
      opacity: 0.9;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 MoodBot</h1>
    
    <div class="status">
      <div id="status">Mood: Happy | Mode: Manual | State: Stopped</div>
    </div>
    
    <h3 style="text-align:center; margin:10px 0;">Movement</h3>
    <div class="controls">
      <button class="btn-fwd" ontouchstart="cmd('W')" onmousedown="cmd('W')">▲</button>
      <button class="btn-left" ontouchstart="cmd('A')" onmousedown="cmd('A')">◄</button>
      <button class="btn-stop" ontouchstart="cmd('X')" onmousedown="cmd('X')">■</button>
      <button class="btn-right" ontouchstart="cmd('D')" onmousedown="cmd('D')">►</button>
      <button class="btn-back" ontouchstart="cmd('S')" onmousedown="cmd('S')">▼</button>
    </div>
    
    <h3 style="text-align:center; margin:10px 0;">Moods</h3>
    <div class="moods">
      <button class="mood-btn" onclick="mood('happy')">😊</button>
      <button class="mood-btn" onclick="mood('excited')">🤩</button>
      <button class="mood-btn" onclick="mood('sad')">😢</button>
      <button class="mood-btn" onclick="mood('angry')">😠</button>
      <button class="mood-btn" onclick="mood('curious')">🤔</button>
      <button class="mood-btn" onclick="mood('sleepy')">😴</button>
    </div>
    
    <button class="auto-btn" onclick="cmd('T')">🔄 Auto Mode</button>
    
    <div class="info">
      Connected to MoodBot<br>
      Watch the OLED screen! 👀
    </div>
  </div>
  
  <script>
    let lastCmd = 0;
    
    function cmd(c) {
      const now = Date.now();
      if (now - lastCmd < 100) return;  // Rate limit
      lastCmd = now;
      
      fetch('/cmd?c=' + c).catch(e => console.log('Cmd failed'));
    }
    
    function mood(m) {
      fetch('/mood?m=' + m).catch(e => console.log('Mood failed'));
    }
    
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          document.getElementById('status').textContent = 
            `Mood: ${d.mood} | Mode: ${d.mode} | State: ${d.state}`;
        })
        .catch(e => {});  // Silently fail
    }
    
    // Update every 3 seconds (reduced from 1 second)
    setInterval(updateStatus, 3000);
    updateStatus();
    
    // Keyboard support
    document.addEventListener('keydown', e => {
      const k = e.key.toUpperCase();
      if (['W','A','S','D','X'].includes(k)) {
        cmd(k);
        e.preventDefault();
      }
    });
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleCommand() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "Missing command");
    return;
  }
  
  // Rate limiting
  if (millis() - lastCommandTime < COMMAND_DELAY) {
    server.send(200, "text/plain", "OK");
    return;
  }
  lastCommandTime = millis();
  
  String cmdStr = server.hasArg("c") ? server.arg("c") : "";
  if (cmdStr.length() > 0) {
    char cmd = cmdStr.charAt(0);
    Serial2.write(cmd);
    Serial2.flush();
    
    Serial.print("Sent: ");
    Serial.println(cmd);
  }
  
  server.send(200, "text/plain", "OK");
}

void handleMood() {
  if (server.hasArg("m")) {
    currentMood = server.arg("m");
    animFrame = 0;
    Serial.print("Mood: ");
    Serial.println(currentMood);
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"mood\":\"" + currentMood + "\",";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"state\":\"" + currentState + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void parseArduinoMessage(String msg) {
  Serial.print("Arduino: ");
  Serial.println(msg);
  
  if (msg.startsWith("STATE:")) {
    currentState = msg.substring(6);
    currentState.toLowerCase();
  } 
  else if (msg.startsWith("MODE:")) {
    currentMode = msg.substring(5);
    currentMode.toLowerCase();
  }
  else if (msg.startsWith("OBSTACLE:")) {
    if (currentMood != "angry") {
      currentMood = "curious";
    }
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Blinking logic
  if (animFrame % 150 == 0 && random(10) < 3) {
    isBlinking = true;
    blinkTimer = 0;
  }
  if (isBlinking) {
    blinkTimer++;
    if (blinkTimer > 8) isBlinking = false;
  }
  
  // Draw face based on mood
  if (currentMood == "happy") drawHappy();
  else if (currentMood == "sad") drawSad();
  else if (currentMood == "angry") drawAngry();
  else if (currentMood == "excited") drawExcited();
  else if (currentMood == "curious") drawCurious();
  else if (currentMood == "sleepy") drawSleepy();
  else drawHappy();
  
  display.display();
}

void drawHappy() {
  // Eyes
  if (isBlinking) {
    display.drawLine(35, 25, 45, 25, SSD1306_WHITE);
    display.drawLine(83, 25, 93, 25, SSD1306_WHITE);
  } else {
    display.fillCircle(40, 25, 6, SSD1306_WHITE);
    display.fillCircle(88, 25, 6, SSD1306_WHITE);
    display.fillCircle(40, 25, 3, SSD1306_BLACK);
    display.fillCircle(88, 25, 3, SSD1306_BLACK);
  }
  
  // Smile
  int bounce = sin(animFrame * 0.08) * 2;
  display.drawCircle(64, 35 + bounce, 18, SSD1306_WHITE);
  display.fillRect(46, 35, 36, 18, SSD1306_BLACK);
}

void drawSad() {
  // Eyes
  if (isBlinking) {
    display.drawLine(35, 25, 45, 25, SSD1306_WHITE);
    display.drawLine(83, 25, 93, 25, SSD1306_WHITE);
  } else {
    display.fillCircle(40, 25, 6, SSD1306_WHITE);
    display.fillCircle(88, 25, 6, SSD1306_WHITE);
    display.fillCircle(40, 25, 3, SSD1306_BLACK);
    display.fillCircle(88, 25, 3, SSD1306_BLACK);
  }
  
  // Tear
  int tear = (animFrame / 3) % 20;
  if (tear < 15) {
    display.fillCircle(48, 28 + tear, 2, SSD1306_WHITE);
  }
  
  // Frown
  display.drawCircle(64, 55, 18, SSD1306_WHITE);
  display.fillRect(46, 38, 36, 17, SSD1306_BLACK);
}

void drawAngry() {
  // Angry eyes
  int shake = (animFrame % 8 < 4) ? 0 : 1;
  display.fillTriangle(35, 20 + shake, 45, 20 + shake, 40, 28 + shake, SSD1306_WHITE);
  display.fillTriangle(83, 20 + shake, 93, 20 + shake, 88, 28 + shake, SSD1306_WHITE);
  
  // Mouth
  display.fillRect(50, 45, 28, 5, SSD1306_WHITE);
}

void drawExcited() {
  // Big eyes
  int sparkle = abs(sin(animFrame * 0.2)) * 2;
  display.fillCircle(40, 25, 8, SSD1306_WHITE);
  display.fillCircle(88, 25, 8, SSD1306_WHITE);
  display.fillCircle(40, 25, 4 + sparkle, SSD1306_BLACK);
  display.fillCircle(88, 25, 4 + sparkle, SSD1306_BLACK);
  
  // Big smile
  display.fillCircle(64, 38, 22, SSD1306_WHITE);
  display.fillRect(42, 25, 44, 22, SSD1306_BLACK);
}

void drawCurious() {
  // Different sized eyes
  display.fillCircle(40, 25, 8, SSD1306_WHITE);
  display.fillCircle(88, 25, 5, SSD1306_WHITE);
  display.fillCircle(40, 25, 4, SSD1306_BLACK);
  display.fillCircle(88, 25, 2, SSD1306_BLACK);
  
  // O mouth
  display.drawCircle(64, 45, 8, SSD1306_WHITE);
  
  // Question mark
  if ((animFrame / 40) % 3 == 0) {
    display.setTextSize(2);
    display.setCursor(105, 10);
    display.print("?");
  }
}

void drawSleepy() {
  // Droopy eyes
  int droop = abs(sin(animFrame * 0.04)) * 2;
  display.drawLine(32, 25 + droop, 48, 25, SSD1306_WHITE);
  display.drawLine(80, 25, 96, 25 + droop, SSD1306_WHITE);
  
  // Small mouth
  display.drawLine(58, 45, 70, 45, SSD1306_WHITE);
  
  // Z's
  int z = (animFrame / 2) % 30;
  if (z < 25) {
    display.setTextSize(1);
    display.setCursor(100, 50 - z);
    display.print("z");
  }
}

void showMessage(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println(msg);
  display.display();
}

/*
 * ========================================
 * QUICK FIXES:
 * ========================================
 * 
 * OLED blank?
 * - Try 0x3D instead of 0x3C in line 35
 * - Check 3.3V power and I2C wiring
 * 
 * WiFi not showing?
 * - Wait 10 seconds after power on
 * - Check Serial Monitor for IP address
 * 
 * Commands not working?
 * - Verify TX/RX wiring (crossed!)
 * - Make sure GND is connected
 * - Disconnect TX/RX when uploading code
 */