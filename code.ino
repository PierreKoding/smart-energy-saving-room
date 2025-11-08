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

// --- Pin definitions ---
const int LED_PIN = 14;
const int LDR_PIN = 34;
const int PIR_PIN = 12;

// --- Stepper pins ---
const int IN1 = 33;
const int IN2 = 25;
const int IN3 = 26;
const int IN4 = 27;

// --- Timing ---
unsigned long lastPIRTime = 0;
const unsigned long ON_DURATION = 10000; // LED & motor aktif 10 detik setelah deteksi PIR

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

static int stepIndex = 0;

// --- Occupancy variables ---
int pirValue;
int lastPirValue = LOW;
unsigned long lastMotionTime = 0;
unsigned long idleThreshold = 10000; // 10s idle before next count
int hourlyCounter = 0;

// --- Power variables ---
float ledPower = 0.007;   // W
float motorPower = 2.4;   // W
float totalPower = 0.0;   // W

// --- Telegram notification control ---
bool motionNotified = false;
unsigned long lastNotificationTime = 0;
const unsigned long NOTIFICATION_COOLDOWN = 15000; // 15 detik agar tidak spam

// --- Blynk control flags ---
bool ledControl = false;
bool motorControl = false;

// === BLYNK SWITCH HANDLERS ===
// V5 → Switch LED control
BLYNK_WRITE(V5) {
  int state = param.asInt();
  ledControl = (state == 1);
  if (ledControl) {
    analogWrite(LED_PIN, 255);  // nyalakan LED penuh
    Serial.println("💡 LED control: ON");
  } else {
    analogWrite(LED_PIN, 0);    // matikan LED
    Serial.println("💡 LED control: OFF");
  }
}

// V6 → Switch Motor control
BLYNK_WRITE(V6) {
  int state = param.asInt();
  motorControl = (state == 1);
  if (motorControl) {
    Serial.println("⚙️ Motor control: ON");
  } else {
    Serial.println("⚙️ Motor control: OFF");
    stopStepper();
  }
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

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
  Blynk.run();

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

      // === Kirim notifikasi Telegram ===
      if (!motionNotified || (currentTime - lastNotificationTime > NOTIFICATION_COOLDOWN)) {
        bot.sendMessage(CHAT_ID, "🚨 Gerakan terdeteksi di ruangan!", "");
        motionNotified = true;
        lastNotificationTime = currentTime;
      }
    }
    lastMotionTime = currentTime;
  }
  lastPirValue = pirValue;

  // --- Control LED & stepper motor ---
  if (pirValue == HIGH) lastPIRTime = currentTime;

  if (!motorControl && !ledControl) {  // mode otomatis
    if (currentTime - lastPIRTime <= ON_DURATION) {
      int brightness = map(ldrValue, 0, 4095, 0, 255);
      analogWrite(LED_PIN, brightness);
      rotateStepper(50, 2);
      totalPower = ledPower + motorPower;
    } else {
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
  
  // --- Send data ke Blynk ---
  Blynk.virtualWrite(V1, pirValue);
  Blynk.virtualWrite(V2, ldrValue);
  Blynk.virtualWrite(V4, totalPower);

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

// === Stepper motor rotation ===
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

void stopStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
