#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Credentials - CHANGE THESE
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

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
int mouthAnimation = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  showMessage("Connecting\nto WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  showMessage("Ready!\n\n" + WiFi.localIP().toString());
  
  delay(2000);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.on("/mood", handleMood);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("Web server started");
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
    }
    .subtitle {
      text-align: center;
      opacity: 0.8;
      margin-bottom: 30px;
    }
    .status {
      background: rgba(255,255,255,0.2);
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      text-align: center;
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
    }
    button:hover {
      background: rgba(255,255,255,0.3);
      transform: scale(1.05);
    }
    button:active {
      transform: scale(0.95);
    }
    .btn-forward { grid-column: 2; }
    .btn-left { grid-column: 1; grid-row: 2; }
    .btn-stop { grid-column: 2; grid-row: 2; background: rgba(255,50,50,0.3); }
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
      font-size: 1em;
    }
    .auto-btn {
      width: 100%;
      margin-top: 20px;
      background: rgba(50,255,50,0.3);
    }
    .info {
      text-align: center;
      margin-top: 20px;
      opacity: 0.7;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 MoodBot</h1>
    <div class="subtitle">Animated Emotional Robot</div>
    
    <div class="status" id="status">
      Mood: <span id="mood">Happy</span> | 
      Mode: <span id="mode">Manual</span> | 
      State: <span id="state">Stopped</span>
    </div>
    
    <h3>Movement Controls</h3>
    <div class="controls">
      <button class="btn-forward" onclick="sendCmd('W')">▲<br>Forward</button>
      <button class="btn-left" onclick="sendCmd('A')">◄<br>Left</button>
      <button class="btn-stop" onclick="sendCmd('X')">■<br>Stop</button>
      <button class="btn-right" onclick="sendCmd('D')">►<br>Right</button>
      <button class="btn-backward" onclick="sendCmd('S')">▼<br>Back</button>
    </div>
    
    <h3>Mood Selection</h3>
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
      Use W/A/S/D keys for keyboard control<br>
      Watch the OLED for animated expressions!
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
        .then(data => console.log(data));
    }
    
    function setMood(mood) {
      fetch('/mood?mood=' + mood)
        .then(r => r.text())
        .then(data => {
          document.getElementById('mood').textContent = mood;
        });
    }
    
    function toggleAuto() {
      fetch('/command?cmd=T')
        .then(r => r.text())
        .then(data => updateStatus());
    }
    
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('mood').textContent = data.mood;
          document.getElementById('mode').textContent = data.mode;
          document.getElementById('state').textContent = data.state;
        });
    }
    
    setInterval(updateStatus, 1000);
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
    server.send(200, "text/plain", "Command sent: " + cmd);
  } else {
    server.send(400, "text/plain", "Missing cmd parameter");
  }
}

void handleMood() {
  if (server.hasArg("mood")) {
    String mood = server.arg("mood");
    currentMood = mood;
    animFrame = 0; // Reset animation
    server.send(200, "text/plain", "Mood set: " + mood);
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
  if (response.startsWith("MODE:")) {
    autoMode = response.indexOf("AUTO") > 0;
  } else if (response.startsWith("MOVING:")) {
    movementState = response.substring(7);
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
  if (movementState == "LEFT") {
    eyeOffsetX = -3;
  } else if (movementState == "RIGHT") {
    eyeOffsetX = 3;
  } else if (movementState == "FORWARD") {
    eyeOffsetY = -2;
  } else if (movementState == "BACKWARD") {
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
    // Blink animation
    display.drawLine(leftEyeX - 8, eyeY, leftEyeX + 8, eyeY, SSD1306_WHITE);
    display.drawLine(rightEyeX - 8, eyeY, rightEyeX + 8, eyeY, SSD1306_WHITE);
  } else {
    // Normal eyes
    display.fillCircle(leftEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(rightEyeX, eyeY, 8, SSD1306_WHITE);
    display.fillCircle(leftEyeX, eyeY, 4, SSD1306_BLACK);
    display.fillCircle(rightEyeX, eyeY, 4, SSD1306_BLACK);
  }
  
  // Animated bouncing smile
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
  
  // Animated tear
  int tearDrop = (animFrame / 4) % 20;
  if (tearDrop < 15) {
    display.fillCircle(leftEyeX + 8, eyeY + 10 + tearDrop, 2, SSD1306_WHITE);
  }
  
  // Sad frown
  display.drawCircle(64, 55, 20, SSD1306_WHITE);
  display.fillRect(44, 35, 40, 20, SSD1306_BLACK);
}

void drawAngryFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  // Angry eyebrows (animated)
  int browShake = (animFrame % 10 < 5) ? 0 : 1;
  display.fillTriangle(leftEyeX - 10, eyeY - 5 + browShake, 
                       leftEyeX + 10, eyeY - 5 + browShake, 
                       leftEyeX, eyeY + 5 + browShake, SSD1306_WHITE);
  display.fillTriangle(rightEyeX - 10, eyeY - 5 + browShake, 
                       rightEyeX + 10, eyeY - 5 + browShake, 
                       rightEyeX, eyeY + 5 + browShake, SSD1306_WHITE);
  
  // Angry mouth (pulsing)
  int pulse = sin(animFrame * 0.2) * 3;
  display.fillRect(50, 45, 28 + pulse, 6, SSD1306_WHITE);
  
  // Anger lines
  if (animFrame % 20 < 10) {
    display.drawLine(20, 10, 25, 5, SSD1306_WHITE);
    display.drawLine(103, 5, 108, 10, SSD1306_WHITE);
  }
}

void drawExcitedFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  // Sparkling eyes
  int sparkle = abs(sin(animFrame * 0.3)) * 3;
  display.fillCircle(leftEyeX, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(rightEyeX, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(leftEyeX, eyeY, 5 + sparkle, SSD1306_BLACK);
  display.fillCircle(rightEyeX, eyeY, 5 + sparkle, SSD1306_BLACK);
  
  // Stars around eyes
  if (animFrame % 30 < 15) {
    drawStar(leftEyeX - 15, eyeY - 10, 3);
    drawStar(rightEyeX + 15, eyeY - 10, 3);
  }
  
  // Big animated smile
  int smileSize = 25 + sin(animFrame * 0.2) * 3;
  display.fillCircle(64, 38, smileSize, SSD1306_WHITE);
  display.fillRect(39, 25, 50, 25, SSD1306_BLACK);
}

void drawCuriousFace() {
  int leftEyeX = 40 + eyeOffsetX;
  int rightEyeX = 88 + eyeOffsetX;
  int eyeY = 25 + eyeOffsetY;
  
  // One eye bigger (looking curious)
  int lookOffset = sin(animFrame * 0.05) * 5;
  display.fillCircle(leftEyeX + lookOffset, eyeY, 10, SSD1306_WHITE);
  display.fillCircle(rightEyeX + lookOffset, eyeY, 6, SSD1306_WHITE);
  display.fillCircle(leftEyeX + lookOffset, eyeY, 5, SSD1306_BLACK);
  display.fillCircle(rightEyeX + lookOffset, eyeY, 3, SSD1306_BLACK);
  
  // Animated O mouth
  int mouthSize = 8 + abs(sin(animFrame * 0.1)) * 3;
  display.drawCircle(64, 45, mouthSize, SSD1306_WHITE);
  
  // Question mark that appears occasionally
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
  
  // Droopy eyes
  int droop = abs(sin(animFrame * 0.05)) * 3;
  display.drawLine(leftEyeX - 10, eyeY + droop, leftEyeX + 10, eyeY, SSD1306_WHITE);
  display.drawLine(rightEyeX - 10, eyeY, rightEyeX + 10, eyeY + droop, SSD1306_WHITE);
  
  // Tiny yawning mouth
  display.drawLine(55, 45, 73, 45, SSD1306_WHITE);
  
  // Animated Z's floating up
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
  display.setCursor(0, 20);
  display.println(msg);
  display.display();
}