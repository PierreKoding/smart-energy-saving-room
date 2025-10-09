// --- Blynk credentials ---
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// --- Wi-Fi credentials ---
const char* SSID = "YOUR_WIFI_NAME";            // replace with your WiFi SSID
const char* PASSWORD = "YOUR_WIFI_PASSWORD";    // replace with your WiFi password

// --- Pin definitions ---
const int LED_PIN = 14;
const int LDR_PIN = 34;
const int PIR_PIN = 12;

// Stepper pins
const int IN1 = 33;
const int IN2 = 25;
const int IN3 = 26;
const int IN4 = 27;

// Timing
unsigned long lastPIRTime = 0;
const unsigned long ON_DURATION = 10000; // 10 seconds after PIR

// Stepper motor half-step sequence
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

static int stepIndex = 0; // for continuous motor

// --- Occupancy counter variables ---
int pirValue;
int lastPirValue = LOW;
unsigned long lastMotionTime = 0;
unsigned long idleThreshold = 10000; // 30s idle required
int hourlyCounter = 0;

// --- Power variables ---
float ledPower = 0.007;   // W
float motorPower = 2.4;   // W
float totalPower = 0.0;   // W

void setup() {
  Serial.begin(115200);

  // Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
}

void loop() {
  Blynk.run();

  pirValue = digitalRead(PIR_PIN);
  int ldrValue = analogRead(LDR_PIN); // 0–4095

  unsigned long currentTime = millis();

  // --- Event-based occupancy detection ---
  if (pirValue == HIGH && lastPirValue == LOW) {
    if (currentTime - lastMotionTime > idleThreshold) {
      hourlyCounter++;
      Blynk.virtualWrite(V3, hourlyCounter);
      Serial.print("New occupancy event, counter: ");
      Serial.println(hourlyCounter);
    }
    lastMotionTime = currentTime;
  }
  lastPirValue = pirValue;

  // --- Control LED + Stepper based on ON_DURATION ---
  if (pirValue == HIGH) lastPIRTime = currentTime;

  if (currentTime - lastPIRTime <= ON_DURATION) {
    int brightness = map(ldrValue, 0, 4095, 0, 255);
    analogWrite(LED_PIN, brightness);

    rotateStepper(50, 2);

    // Power ON
    totalPower = ledPower + motorPower;
  } else {
    analogWrite(LED_PIN, 0);
    totalPower = 0.0; // nothing running
  }

  // --- Send data to Blynk ---
  Blynk.virtualWrite(V1, pirValue);
  Blynk.virtualWrite(V2, ldrValue);
  Blynk.virtualWrite(V4, totalPower); // send instantaneous power (W)

  // Debug
  Serial.print("PIR: ");
  Serial.print(pirValue);
  Serial.print(" | LDR: ");
  Serial.print(ldrValue);
  Serial.print(" | Counter: ");
  Serial.print(hourlyCounter);
  Serial.print(" | Power (W): ");
  Serial.println(totalPower);

  delay(100);
}

// --- Stepper motor rotation ---
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
