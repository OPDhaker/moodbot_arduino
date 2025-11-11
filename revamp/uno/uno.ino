/*
 * ========================================
 *  MOODBOT - ARDUINO UNO (REVAMPED V2)
 * ========================================
 * Simplified, stable motor control without PWM
 * Upload this to Arduino UNO
 * 
 * WIRING (SAME AS BEFORE):
 * L298N Motor Driver:
 *   IN1 → Pin 2  |  Motor A (Left)  → OUT1, OUT2
 *   IN2 → Pin 3  |  Motor B (Right) → OUT3, OUT4
 *   IN3 → Pin 4  |  12V → Battery (7-12V)
 *   IN4 → Pin 5  |  GND → Arduino GND + Battery GND
 *   ENA → Remove jumper and leave disconnected
 *   ENB → Remove jumper and leave disconnected
 * 
 * HC-SR04 Ultrasonic:
 *   TRIG → Pin 7  |  VCC → 5V
 *   ECHO → Pin 8  |  GND → GND
 * 
 * ESP32 Serial:
 *   TX (Pin 1) → ESP32 GPIO 16
 *   RX (Pin 0) → ESP32 GPIO 17
 *   GND → ESP32 GND
 */

// Motor Driver Pins (no ENA/ENB needed!)
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5

// Ultrasonic Sensor
#define TRIG_PIN 7
#define ECHO_PIN 8

// Settings
#define OBSTACLE_DISTANCE 25     // cm
#define AUTO_CHECK_INTERVAL 200  // ms
#define COMMAND_TIMEOUT 100      // ms

// State tracking
bool autoMode = false;
unsigned long lastAutoCheck = 0;
unsigned long lastCommandTime = 0;
char currentCommand = 'X';  // X = stopped

void setup() {
  Serial.begin(9600);
  
  // Motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Start stopped
  stopMotors();
  
  // Initial status
  delay(500);
  Serial.println("READY");
  Serial.flush();
}

void loop() {
  // Read commands from ESP32
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Only process valid commands
    if (cmd == 'W' || cmd == 'A' || cmd == 'S' || cmd == 'D' || 
        cmd == 'X' || cmd == 'T') {
      handleCommand(cmd);
      lastCommandTime = millis();
    }
  }
  
  // Auto mode navigation
  if (autoMode) {
    if (millis() - lastAutoCheck >= AUTO_CHECK_INTERVAL) {
      lastAutoCheck = millis();
      autoNavigate();
    }
  }
  
  // Safety: stop if no command received in manual mode for a while
  if (!autoMode && currentCommand != 'X') {
    if (millis() - lastCommandTime > 5000) {  // 5 seconds timeout
      stopMotors();
      currentCommand = 'X';
    }
  }
}

void handleCommand(char cmd) {
  currentCommand = cmd;
  
  switch(cmd) {
    case 'W':  // Forward
      moveForward();
      Serial.println("STATE:FORWARD");
      break;
      
    case 'S':  // Backward
      moveBackward();
      Serial.println("STATE:BACKWARD");
      break;
      
    case 'A':  // Left
      turnLeft();
      Serial.println("STATE:LEFT");
      break;
      
    case 'D':  // Right
      turnRight();
      Serial.println("STATE:RIGHT");
      break;
      
    case 'X':  // Stop
      stopMotors();
      Serial.println("STATE:STOP");
      break;
      
    case 'T':  // Toggle auto mode
      autoMode = !autoMode;
      if (autoMode) {
        Serial.println("MODE:AUTO");
        lastAutoCheck = millis();  // Start immediately
      } else {
        Serial.println("MODE:MANUAL");
        stopMotors();
        currentCommand = 'X';
      }
      break;
  }
  
  Serial.flush();
}

void autoNavigate() {
  int distance = getDistance();
  
  // Check for obstacle
  if (distance > 0 && distance < OBSTACLE_DISTANCE) {
    // Obstacle detected!
    Serial.println("OBSTACLE:YES");
    
    stopMotors();
    delay(200);
    
    // Back up
    moveBackward();
    delay(400);
    stopMotors();
    delay(100);
    
    // Turn random direction
    if (random(2) == 0) {
      turnLeft();
    } else {
      turnRight();
    }
    delay(600);
    stopMotors();
    delay(100);
    
  } else {
    // Path clear - move forward
    if (currentCommand != 'W') {
      moveForward();
      currentCommand = 'W';
    }
  }
}

int getDistance() {
  // Send pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo with timeout
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if (duration == 0) return 999;  // No obstacle
  
  int distance = duration * 0.034 / 2;
  return distance;
}

// ============================================
// MOTOR CONTROL FUNCTIONS (Full Speed)
// ============================================

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  // Left motor backward, right motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  // Left motor forward, right motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

/*
 * ========================================
 * WIRING NOTES:
 * ========================================
 * 
 * L298N Setup (NO ENA/ENB):
 * - Remove BOTH jumpers from ENA and ENB
 * - Leave ENA and ENB pins unconnected
 * - Motors run at full speed (no PWM control)
 * - Connect 12V jumper if using <12V battery
 * 
 * If motor spins wrong direction:
 * - Swap the two wires on that motor's OUT terminals
 * 
 * Power:
 * - Battery 7.4V+ to L298N 12V terminal
 * - Battery GND to L298N GND
 * - Arduino VIN to battery + (or USB)
 * - Connect ALL GNDs together!
 */