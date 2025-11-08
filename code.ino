/******************************************************
 * COMMUNICATION PROTOCOL CONFIGURATION
 * ----------------------------------------------------
 * Blynk: MQTT over TCP
 * Server: blynk.cloud
 * Data sent: Virtual Pins V1–V4 (PIR, LDR, Counter, Power)
 * Telegram: HTTPS requests via UniversalTelegramBot library
 * API Endpoint: https://api.telegram.org/bot<token>/sendMessage
 * Wi-Fi: 2.4GHz network, WPA2
 ******************************************************/

// --- Blynk credentials ---
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"

// --- Telegram credentials ---
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_TELEGRAM_CHAT_ID"

// --- Wi-Fi credentials ---
const char* WIFI_SSID = "YOUR_WIFI_NAME";          // replace with your WiFi SSID
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // replace with your WiFi password

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>

WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

// --- Sensor and actuator pin definitions ---
const int LED_PIN = 14;   // PWM output for LED brightness control
const int LDR_PIN = 34;   // Analog input for light intensity
const int PIR_PIN = 12;   // Digital input from motion sensor (PIR)


// --- Stepper motor control pins (IN1–IN4 sequence) ---
const int IN1 = 33;
const int IN2 = 25;
const int IN3 = 26;
const int IN4 = 27;

// --- Stepper motor half-step sequence ---
int stepSequence[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

static int stepIndex = 0;  // Tracks current step in sequence

// --- Timing control for motion and light ---
unsigned long lastPIRTime = 0;             // Last time motion was detected
const unsigned long ON_DURATION = 10000;   // Duration to keep LED/motor on after motion (ms)
unsigned long lastMotionTime = 0;          // Last motion detection timestamp
unsigned long idleThreshold = 10000;       // Threshold for counting new motion events (ms)

// --- Occupancy and motion tracking ---
int pirValue;                              // Current PIR sensor reading
int lastPirValue = LOW;                    // Previous PIR sensor state
int hourlyCounter = 0;                     // Tracks number of motion events per hour

// --- Power monitoring (in watts) ---
float ledPower = 0.007;                    // LED power consumption (W)
float motorPower = 2.4;                    // Stepper motor power (W)
float totalPower = 0.0;                    // Total system power usage (W)

// --- Telegram notification control ---
bool motionNotified = false;               // Prevents duplicate alerts
unsigned long lastNotificationTime = 0;    // Last notification timestamp
const unsigned long NOTIFICATION_COOLDOWN = 15000;  // Cooldown before next alert (ms)

// --- Blynk control flags ---
bool ledControl = false;                   // Manual LED control (from dashboard)
bool motorControl = false;                 // Manual motor control (from dashboard)

// === BLYNK SWITCH HANDLERS ===
// --- Manual LED control (Virtual Pin V5) ---
BLYNK_WRITE(V5) {
  int state = param.asInt();
  ledControl = (state == 1);

  if (ledControl) {
    analogWrite(LED_PIN, 255);  // Full brightness
    Serial.println("LED manual control: ON");
  } else {
    analogWrite(LED_PIN, 0);    // Turn off
    Serial.println("LED manual control: OFF");
  }
}


// --- Manual Motor control (Virtual Pin V6) ---
BLYNK_WRITE(V6) {
  int state = param.asInt();
  motorControl = (state == 1);

  if (motorControl) {
    Serial.println("Motor manual control: ON");
  } else {
    Serial.println("Motor manual control: OFF");
    stopStepper();  // Ensure motor stops immediately
  }
}


// === SETUP ===
void setup() {
  Serial.begin(115200);  // Initialize serial monitor

  // WiFi + Blynk setup
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");

  // Telegram secure client setup
  secureClient.setInsecure(); // disable SSL verification (simplify connection)
  bot.sendMessage(CHAT_ID, "✅ ESP32 berhasil terhubung ke WiFi & Telegram!", "");

  // Blynk setup
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);

  // Pin setup
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.println("System Ready.");
}

// === LOOP ===
void loop() {
  Blynk.run();  // Maintain Blynk connection and handle dashboard updates

  // Read Sensors
  pirValue = digitalRead(PIR_PIN);
  int ldrValue = analogRead(LDR_PIN);
  unsigned long currentTime = millis();

  // --- Event-based occupancy detection ---
  if (pirValue == HIGH && lastPirValue == LOW) {
    if (currentTime - lastMotionTime > idleThreshold) {
      hourlyCounter++;
      Blynk.virtualWrite(V3, hourlyCounter);
      Serial.print("New occupancy event, counter: ");
      Serial.println(hourlyCounter);

      // === Send Telegram alert if cooldown elapsed ===
      if (!motionNotified || (currentTime - lastNotificationTime > NOTIFICATION_COOLDOWN)) {
        bot.sendMessage(CHAT_ID, "🚨 Gerakan terdeteksi di ruangan!", "");
        motionNotified = true;
        lastNotificationTime = currentTime;
      }
    }
    lastMotionTime = currentTime;
  }
  lastPirValue = pirValue;

  // --- LED and motor control logic ---
  if (pirValue == HIGH) lastPIRTime = currentTime;

  // --- Automatic mode (no manual override active) ---
  if (!motorControl && !ledControl) {  // mode otomatis
    if (currentTime - lastPIRTime <= ON_DURATION) {
      // Adjust LED brightness based on ambient light
      int brightness = map(ldrValue, 0, 4095, 0, 255);
      analogWrite(LED_PIN, brightness);
      rotateStepper(50, 2);
      // Update total power usage
      totalPower = ledPower + motorPower;
    } else {
      // Turn off components when idle
      analogWrite(LED_PIN, 0);
      stopStepper();
      totalPower = 0.0;
    }
  } else {
    if (motorControl) {
      rotateStepper(50, 2);
    } else {
      stopStepper();
    }
  }
  
  // --- Send live data to Blynk dashboard ---
  Blynk.virtualWrite(V1, pirValue);     // Occupancy status
  Blynk.virtualWrite(V2, ldrValue);     // Light level
  Blynk.virtualWrite(V4, totalPower);   // Power usage

  // --- Debug log ---
  Serial.print("PIR: ");
  Serial.print(pirValue);
  Serial.print(" | LDR: ");
  Serial.print(ldrValue);
  Serial.print(" | Counter: ");
  Serial.print(hourlyCounter);
  Serial.print(" | Power(W): ");
  Serial.println(totalPower);

  delay(100);
}

// --- Rotate stepper motor for given number of steps and delay ---
void rotateStepper(int steps, int delayTime) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(IN1, stepSequence[stepIndex][0]);
    digitalWrite(IN2, stepSequence[stepIndex][1]);
    digitalWrite(IN3, stepSequence[stepIndex][2]);
    digitalWrite(IN4, stepSequence[stepIndex][3]);

    stepIndex = (stepIndex + 1) % 8;
    delay(delayTime);
  }
}
// --- Immediately stop stepper motor ---
void stopStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
