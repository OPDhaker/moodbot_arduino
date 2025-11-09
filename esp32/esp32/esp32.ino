/*
 * ========================================
 *  MOODBOT - ESP32 CODE  
 * ========================================
 * Handles: WiFi Access Point, Animated OLED display, Serial to Arduino
 * Upload this code to your ESP32
 * 
 * WIRING CONNECTIONS:
 * OLED Display (I2C):
 *   SDA → GPIO 21
 *   SCL → GPIO 22  
 *   VCC → 3.3V
 *   GND → GND
 * 
 * Arduino Communication:
 *   ESP32 RX2 (GPIO 16) → Arduino TX (Pin 1)
 *   ESP32 TX2 (GPIO 17) → Arduino RX (Pin 0)
 *   ESP32 GND → Arduino GND (IMPORTANT!)
 * 
 * WiFi Access Point Created:
 *   Network Name: MoodBot
 *   Password: 12345678
 *   IP Address: 192.168.4.1
 * 
 * HOW TO CONNECT:
 * 1. Upload this code to ESP32
 * 2. Wait 10 seconds for boot
 * 3. On your phone/laptop, connect to WiFi "MoodBot" 
 * 4. Password: 12345678
 * 5. Open browser and go to: http://192.168.4.1
 * 6. Control your robot!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Access Point Settings (ESP32 creates its own network)
const char* ssid = "MoodBot";           // WiFi name people will see
const char* password = "12345678";      // WiFi password
IPAddress local_IP(192, 168, 4, 1);     // ESP32 IP address
IPAddress gateway(192, 168, 4, 1);      // Gateway
IPAddress subnet(255, 255, 255, 0);     // Subnet mask

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
String movementState = "stopped";
bool autoMode = false;

// Animation variables
unsigned long lastAnimUpdate = 0;
int animFrame = 0;
int blinkCounter = 0;
bool isBlinking = false;
int eyeOffsetX = 0;
int eyeOffsetY = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    Serial.println("Check OLED wiring!");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Create WiFi Access Point
  Serial.println("Creating WiFi Access Point...");
  showMessage("Starting\nMoodBot...");
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n========================================");
  Serial.println("MoodBot WiFi Access Point Created!");
  Serial.println("========================================");
  Serial.print("Network Name: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.println("========================================");
  Serial.println("\nConnect your phone/laptop to WiFi:");
  Serial.println("1. Look for 'MoodBot' in WiFi list");
  Serial.println("2. Enter password: 12345678");
  Serial.println("3. Open browser: http://192.168.4.1");
  Serial.println("========================================\n");
  
  showMessage("WiFi: MoodBot\nPass: 12345678\n\nGo to:\n192.168.4.1");
  delay(3000);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.on("/mood", handleMood);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("Ready for connections...\n");
  
  delay(1000);
  currentMood = "happy";
}

void loop() {
  server.handleClient();
  
  // Check for status updates from Arduino
  if (Serial2.available()) {
    String response = Serial2.readStringUntil('\n');
    response.trim();
    parseArduinoResponse(response);
  }
  
  // Update animations at 60 FPS
  if (millis() - lastAnimUpdate > 16) {
    lastAnimUpdate = millis();
    animFrame++;
    updateAnimation();
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
      font-family: 'Segoe UI', Tahoma, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 20px;
      min-height: 100vh;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: rgba(255,255,255,0.1);
      backdrop-filter: blur(10px);
      border-radius: 20px;
      padding: 30px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
    }
    h1 {
      text-align: center;
      margin-bottom: 10px;
      font-size: 2.5em;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    .subtitle {
      text-align: center;
      opacity: 0.9;
      margin-bottom: 30px;
      font-size: 1.1em;
    }
    .status {
      background: rgba(255,255,255,0.2);
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      text-align: center;
      font-weight: bold;
      border: 2px solid rgba(255,255,255,0.3);
    }
    .section-title {
      margin-top: 25px;
      margin-bottom: 15px;
      font-size: 1.3em;
      font-weight: bold;
      text-align: center;
      text-shadow: 1px 1px 2px rgba(0,0,0,0.3);
    }
    .controls {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin: 20px 0;
    }
    button {
      background: rgba(255,255,255,0.2);
      border: 2px solid white;
      color: white;
      padding: 20px;
      font-size: 1.2em;
      border-radius: 10px;
      cursor: pointer;
      transition: all 0.3s;
      font-weight: bold;
      box-shadow: 0 4px 6px rgba(0,0,0,0.2);
    }
    button:hover {
      background: rgba(255,255,255,0.3);
      transform: scale(1.05);
      box-shadow: 0 6px 12px rgba(0,0,0,0.3);
    }
    button:active {
      transform: scale(0.95);
    }
    .btn-forward { grid-column: 2; }
    .btn-left { grid-column: 1; grid-row: 2; }
    .btn-stop { 
      grid-column: 2; 
      grid-row: 2; 
      background: rgba(255,50,50,0.4);
      border-color: #ff6b6b;
    }
    .btn-right { grid-column: 3; grid-row: 2; }
    .btn-backward { grid-column: 2; grid-row: 3; }
    .mood-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin: 20px 0;
    }
    .mood-btn {
      padding: 15px;
      font-size: 1.1em;
    }
    .auto-btn {
      width: 100%;
      margin-top: 20px;
      background: rgba(50,255,50,0.3);
      border-color: #51cf66;
      font-size: 1.2em;
      padding: 18px;
    }
    .info {
      text-align: center;
      margin-top: 20px;
      opacity: 0.8;
      font-size: 0.95em;
      line-height: 1.6;
      background: rgba(0,0,0,0.2);
      padding: 15px;
      border-radius: 10px;
    }
    .connection-info {
      background: rgba(255,255,255,0.15);
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 20px;
      font-size: 0.9em;
      text-align: center;
      border: 1px solid rgba(255,255,255,0.3);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 MoodBot</h1>
    <div class="subtitle">Animated Emotional Robot</div>
    
    <div class="connection-info">
      ✅ Connected to MoodBot WiFi<br>
      📡 IP: 192.168.4.1
    </div>
    
    <div class="status" id="status">
      Mood: <span id="mood">Happy</span> | 
      Mode: <span id="mode">Manual</span><br>
      State: <span id="state">Stopped</span>
    </div>
    
    <div class="section-title">🎮 Movement Controls</div>
    <div class="controls">
      <button class="btn-forward" onclick="sendCmd('W')">▲<br>Forward</button>
      <button class="btn-left" onclick="sendCmd('A')">◄<br>Left</button>
      <button class="btn-stop" onclick="sendCmd('X')">■<br>Stop</button>
      <button class="btn-right" onclick="sendCmd('D')">►<br>Right</button>
      <button class="btn-backward" onclick="sendCmd('S')">▼<br>Back</button>
    </div>
    
    <div class="section-title">🎭 Mood Selection</div>
    <div class="mood-grid">
      <button class="mood-btn" onclick="setMood('happy')">😊 Happy</button>
      <button class="mood-btn" onclick="setMood('excited')">🤩 Excited</button>
      <button class="mood-btn" onclick="setMood('sad')">😢 Sad</button>
      <button class="mood-btn" onclick="setMood('angry')">😠 Angry</button>
      <button class="mood-btn" onclick="setMood('curious')">🤔 Curious</button>
      <button class="mood-btn" onclick="setMood('sleepy')">😴 Sleepy</button>
    </div>
    
    <button class="auto-btn" onclick="toggleAuto()">🔄 Toggle Auto Mode</button>
    
    <div class="info">
      ⌨️ Use W/A/S/D keys for keyboard control<br>
      👀 Watch the OLED for animated expressions!<br>
      🤖 Auto mode enables obstacle avoidance
    </div>
  </div>
  
  <script>
    // Keyboard controls
    document.addEventListener('keydown', (e) => {
      const key = e.key.toUpperCase();
      if (['W','A','S','D','X'].includes(key)) {
        sendCmd(key);
        e.preventDefault();
      }
    });
    
    function sendCmd(cmd) {
      fetch('/command?cmd=' + cmd)
        .then(r => r.text())
        .then(data => console.log(data))
        .catch(err => console.error('Error:', err));
    }
    
    function setMood(mood) {
      fetch('/mood?mood=' + mood)
        .then(r => r.text())
        .then(data => {
          document.getElementById('mood').textContent = mood.charAt(0).toUpperCase() + mood.slice(1);
        })
        .catch(err => console.error('Error:', err));
    }
    
    function toggleAuto() {
      fetch('/command?cmd=T')
        .then(r => r.text())
        .then(data => {
          setTimeout(updateStatus, 100);
        })
        .catch(err => console.error('Error:', err));
    }
    
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('mood').textContent = data.mood.charAt(0).toUpperCase() + data.mood.slice(1);
          document.getElementById('mode').textContent = data.mode;
          document.getElementById('state').textContent = data.state;
        })
        .catch(err => console.error('Error:', err));
    }
    
    // Update status every second
    setInterval(updateStatus, 1000);
    
    // Initial status update
    setTimeout(updateStatus, 500);
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleCommand() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    Serial2.println(cmd.charAt(0));
    Serial.print("Command sent to Arduino: ");
    Serial.println(cmd);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing cmd parameter");
  }
}

void handleMood() {
  if (server.hasArg("mood")) {
    String mood = server.arg("mood");
    currentMood = mood;
    animFrame = 0; // Reset animation
    Serial.print("Mood changed to: ");
    Serial.println(mood);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing mood parameter");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"mood\":\"" + currentMood + "\",";
  json += "\"mode\":\"" + String(autoMode ? "Auto" : "Manual") + "\",";
  json += "\"state\":\"" + movementState + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void parseArduinoResponse(String response) {
  Serial.print("Arduino: ");
  Serial.println(response);
  
  if (response.startsWith("MODE:")) {
    autoMode = response.indexOf("AUTO") > 0;
  } else if (response.startsWith("MOVING:")) {
    movementState = response.substring(7);
    movementState.toLowerCase();
  } else if (response.startsWith("OBSTACLE:")) {
    if (currentMood != "angry") {
      currentMood = "curious";
      animFrame = 0;
    }
  }
}

void updateAnimation() {
  display.clearDisplay();
  
  // Random blinking
  if (animFrame % 120 == 0 && random(10) < 3) {
    isBlinking = true;
    blinkCounter = 0;
  }
  if (isBlinking) {
    blinkCounter++;
    if (blinkCounter > 10) isBlinking = false;
  }
  
  // Eye tracking movement based on robot state
  if (movementState == "left") {
    eyeOffsetX = -3;
  } else if (movementState == "right") {
    eyeOffsetX = 3;
  } else if (movementState == "forward") {
    eyeOffsetY = -2;
  } else if (movementState == "backward") {
    eyeOffsetY = 2;
  } else {
    eyeOffsetX = 0;
    eyeOffsetY = 0;
  }
  
  if (currentMood == "happy") {
    drawHappyFace();
  } else if (currentMood == "sad") {
    drawSadFace();
  } else if (currentMood == "angry") {
    drawAngryFace();
  } else if (currentMood == "excited") {
    drawExcitedFace();
  } else if (currentMood == "curious") {
    drawCuriousFace();
  } else if (currentMood == "sleepy") {
    drawSleepyFace();
  }
  
  display.display();
}

void drawHappyFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  if (isBlinking) {
    display.drawLine(leftEyeX - 8, eyeY, leftEyeX + 8, eyeY, SSD1306_WHITE);
    display.drawLine(rightEyeX - 8, eyeY, rightEyeX + 8, eyeY, SSD1306_WHITE);
  } else {
    display.fillCircle(leftEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(rightEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(leftEyeX, eyeY, 4, SSD1306_BLACK);
    display.fillCircle(rightEyeX, eyeY, 4, SSD1306_BLACK);
  }
  
  int bounce = sin(animFrame * 0.1) * 2;
  for (int i = 0; i < 3; i++) {
    display.drawCircle(64, 35 + bounce, 20 + i, SSD1306_WHITE);
  }
  display.fillRect(44, 35, 40, 20, SSD1306_BLACK);
}

void drawSadFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  if (isBlinking) {
    display.drawLine(leftEyeX - 8, eyeY, leftEyeX + 8, eyeY, SSD1306_WHITE);
    display.drawLine(rightEyeX - 8, eyeY, rightEyeX + 8, eyeY, SSD1306_WHITE);
  } else {
    display.fillCircle(leftEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(rightEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(leftEyeX, eyeY, 4, SSD1306_BLACK);
    display.fillCircle(rightEyeX, eyeY, 4, SSD1306_BLACK);
  }
  
  int tearDrop = (animFrame / 4) % 20;
  if (tearDrop < 15) {
    display.fillCircle(leftEyeX + 8, eyeY + 10 + tearDrop, 2, SSD1306_WHITE);
  }
  
  display.drawCircle(64, 55, 20, SSD1306_WHITE);
  display.fillRect(44, 35, 40, 20, SSD1306_BLACK);
}

void drawAngryFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  int browShake = (animFrame % 10 < 5) ? 0 : 1;
  display.fillTriangle(leftEyeX - 10, eyeY - 5 + browShake, 
                       leftEyeX + 10, eyeY - 5 + browShake, 
                       leftEyeX, eyeY + 5 + browShake, SSD1306_WHITE);
  display.fillTriangle(rightEyeX - 10, eyeY - 5 + browShake, 
                       rightEyeX + 10, eyeY - 5 + browShake, 
                       rightEyeX, eyeY + 5 + browShake, SSD1306_WHITE);
  
  int pulse = sin(animFrame * 0.2) * 3;
  display.fillRect(50, 45, 28 + pulse, 6, SSD1306_WHITE);
  
  if (animFrame % 20 < 10) {
    display.drawLine(20, 10, 25, 5, SSD1306_WHITE);
    display.drawLine(103, 5, 108, 10, SSD1306_WHITE);
  }
}

void drawExcitedFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  int sparkle = abs(sin(animFrame * 0.3)) * 3;
  display.fillCircle(leftEyeX, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(rightEyeX, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(leftEyeX, eyeY, 5 + sparkle, SSD1306_BLACK);
  display.fillCircle(rightEyeX, eyeY, 5 + sparkle, SSD1306_BLACK);
  
  if (animFrame % 30 < 15) {
    drawStar(leftEyeX - 15, eyeY - 10, 3);
    drawStar(rightEyeX + 15, eyeY - 10, 3);
  }
  
  int smileSize = 25 + sin(animFrame * 0.2) * 3;
  display.fillCircle(64, 38, smileSize, SSD1306_WHITE);
  display.fillRect(39, 25, 50, 25, SSD1306_BLACK);
}

void drawCuriousFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  int lookOffset = sin(animFrame * 0.05) * 5;
  display.fillCircle(leftEyeX + lookOffset, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(rightEyeX + lookOffset, eyeY, 6, SSD1306_WHITE);
  display.fillCircle(leftEyeX + lookOffset, eyeY, 5, SSD1306_BLACK);
  display.fillCircle(rightEyeX + lookOffset, eyeY, 3, SSD1306_BLACK);
  
  int mouthSize = 8 + abs(sin(animFrame * 0.1)) * 3;
  display.drawCircle(64, 45, mouthSize, SSD1306_WHITE);
  
  if ((animFrame / 60) % 4 == 0) {
    display.setTextSize(2);
    display.setCursor(105, 10);
    display.print("?");
  }
}

void drawSleepyFace() {
  int leftEyeX = 40;
  int rightEyeX = 88;
  int eyeY = 25;
  
  int droop = abs(sin(animFrame * 0.05)) * 3;
  display.drawLine(leftEyeX - 10, eyeY + droop, leftEyeX + 10, eyeY, SSD1306_WHITE);
  display.drawLine(rightEyeX - 10, eyeY, rightEyeX + 10, eyeY + droop, SSD1306_WHITE);
  
  display.drawLine(55, 45, 73, 45, SSD1306_WHITE);
  
  int z1 = (animFrame / 2) % 40;
  int z2 = ((animFrame / 2) + 15) % 40;
  int z3 = ((animFrame / 2) + 30) % 40;
  
  if (z1 < 35) {
    display.setTextSize(1);
    display.setCursor(100, 50 - z1);
    display.print("z");
  }
  if (z2 < 35) {
    display.setTextSize(1 + (z2 / 20));
    display.setCursor(105, 50 - z2);
    display.print("z");
  }
  if (z3 < 35) {
    display.setTextSize(2);
    display.setCursor(108, 50 - z3);
    display.print("z");
  }
}

void drawStar(int x, int y, int size) {
  display.drawLine(x, y - size, x, y + size, SSD1306_WHITE);
  display.drawLine(x - size, y, x + size, y, SSD1306_WHITE);
  display.drawLine(x - size/2, y - size/2, x + size/2, y + size/2, SSD1306_WHITE);
  display.drawLine(x + size/2, y - size/2, x - size/2, y + size/2, SSD1306_WHITE);
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
 * TROUBLESHOOTING:
 * ========================================
 * 
 * Can't see "MoodBot" WiFi network?
 * - Wait 10-15 seconds after powering ESP32
 * - Check Serial Monitor for "Access Point Created"
 * - ESP32 must be powered properly (USB or 5V)
 * 
 * OLED display blank?
 * - Check I2C wiring: SDA=21, SCL=22
 * - Verify OLED is getting 3.3V power
 * - Try changing 0x3C to 0x3D in line 49
 * 
 * Can't control robot from webpage?
 * - Verify Arduino and ESP32 GND are connected
 * - Check TX/RX connections (cross-wired)
 * - Open Serial Monitor to see communication
 * 
 * Motors not responding?
 * - Problem is likely on Arduino side
 * - Check Arduino Serial Monitor for commands
 * - Verify L298N has 12V battery power
 */