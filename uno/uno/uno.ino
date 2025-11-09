/*
 * ========================================
 *  MOODBOT - ARDUINO UNO CODE
 * ========================================
 * Handles: Motor control, Ultrasonic sensor, Serial commands from ESP32
 * Upload this code to your Arduino UNO
 * 
 * WIRING CONNECTIONS:
 * L298N Motor Driver:
 *   IN1 → Pin 2  |  ENA → Pin 9 (PWM)
 *   IN2 → Pin 3  |  ENB → Pin 10 (PWM)
 *   IN3 → Pin 4  |  12V → Battery (7-12V)
 *   IN4 → Pin 5  |  GND → Arduino GND
 *   Motor A (Left)  → L298N OUT1, OUT2
 *   Motor B (Right) → L298N OUT3, OUT4
 * 
 * HC-SR04 Ultrasonic Sensor:
 *   TRIG → Pin 7
 *   ECHO → Pin 8
 *   VCC  → 5V
 *   GND  → GND
 * 
 * ESP32 Communication:
 *   Arduino TX (Pin 1) → ESP32 RX2 (GPIO 16)
 *   Arduino RX (Pin 0) → ESP32 TX2 (GPIO 17)
 *   Arduino GND → ESP32 GND (IMPORTANT!)
 */

// L298N Motor Driver Pins
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5
#define ENA 9  // PWM for left motor speed
#define ENB 10 // PWM for right motor speed

// HC-SR04 Ultrasonic Sensor
#define TRIG_PIN 7
#define ECHO_PIN 8

// Settings
#define OBSTACLE_DISTANCE 20  // cm - stop if obstacle closer than this
#define MOTOR_SPEED 200       // 0-255 (adjust if motors too fast/slow)
#define AUTO_CHECK_INTERVAL 100 // ms

bool autoMode = false;
unsigned long lastAutoCheck = 0;

void setup() {
  Serial.begin(9600);
  
  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  stopMotors();
  
  Serial.println("Arduino Ready!");
}

void loop() {
  // Check for Serial commands from ESP32
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // Auto mode obstacle avoidance
  if (autoMode && millis() - lastAutoCheck > AUTO_CHECK_INTERVAL) {
    lastAutoCheck = millis();
    autoNavigate();
  }
}

void handleCommand(char cmd) {
  switch(cmd) {
    case 'W': // Forward
      moveForward();
      Serial.println("MOVING:FORWARD");
      break;
    case 'S': // Backward
      moveBackward();
      Serial.println("MOVING:BACKWARD");
      break;
    case 'A': // Left
      turnLeft();
      Serial.println("MOVING:LEFT");
      break;
    case 'D': // Right
      turnRight();
      Serial.println("MOVING:RIGHT");
      break;
    case 'X': // Stop
      stopMotors();
      Serial.println("MOVING:STOP");
      break;
    case 'T': // Toggle auto mode
      autoMode = !autoMode;
      if (autoMode) {
        Serial.println("MODE:AUTO");
      } else {
        Serial.println("MODE:MANUAL");
        stopMotors();
      }
      break;
    case '?': // Status request
      Serial.print("STATUS:");
      Serial.print(autoMode ? "AUTO:" : "MANUAL:");
      Serial.println(getDistance());
      break;
  }
}

void autoNavigate() {
  int distance = getDistance();
  
  if (distance < OBSTACLE_DISTANCE && distance > 0) {
    // Obstacle detected
    Serial.println("OBSTACLE:DETECTED");
    stopMotors();
    delay(300);
    
    // Back up a bit
    moveBackward();
    delay(400);
    
    // Turn randomly left or right
    if (random(2) == 0) {
      turnLeft();
    } else {
      turnRight();
    }
    delay(500);
  } else {
    // Path clear, move forward
    moveForward();
  }
}

int getDistance() {
  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return -1; // No echo
  
  int distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

/*
 * ========================================
 * TROUBLESHOOTING:
 * ========================================
 * 
 * Motors not moving?
 * - Check 12V battery is connected to L298N
 * - Verify motor wires are connected to OUT1-OUT4
 * - Try increasing MOTOR_SPEED value (line 38)
 * 
 * Motors spinning opposite direction?
 * - Swap the two wires on that motor
 * 
 * Ultrasonic not working?
 * - Check TRIG on Pin 7, ECHO on Pin 8
 * - Verify 5V and GND connections
 * 
 * No Serial communication with ESP32?
 * - Make sure GND is connected between both boards
 * - Arduino TX → ESP32 GPIO 16
 * - Arduino RX → ESP32 GPIO 17
 * - Disconnect these wires during upload!
 */