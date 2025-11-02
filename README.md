A Smart Energy-Saving Room system built using an ESP32, PIR sensor, LDR, LED, and stepper motor. The system detects room occupancy and ambient light intensity to automate lighting and demonstrate mechanical motion. Real-time sensor data and power usage are transmitted to a Blynk dashboard, allowing monitoring and basic energy management.


**🚀 Features**
- Automatic Occupancy Detection: PIR sensor detects motion and increases an hourly counter.
- Light-Based LED Control: LED brightness adjusts automatically based on LDR readings.
- Mechanical Demonstration: Stepper motor activates when occupancy is detected.
- Power Monitoring: Calculates total power usage of LED and motor in watts.
- Blynk Integration: Sends real-time data to the Blynk dashboard for live visualization.
- Telegram integration for automatic motion alerts.
- Manual LED and motor override switches through Blynk.

**🛠️ Setup Instructions**

1. Clone this repository
```
git clone https://github.com/your-username/smart-energy-saving-room.git
```
3. Open the project in Arduino IDE or PlatformIO.

4. Install the required library:
- Blynk (for ESP32)
- UniversalTelegramBot

4. Replace Blynk and Wi-Fi credentials inside the code:
```
#define BLYNK_TEMPLATE_ID "TMPL6bwT0r3FU"
#define BLYNK_TEMPLATE_NAME "House occupancy detector"
#define BLYNK_AUTH_TOKEN "your_auth_token"
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_TELEGRAM_CHAT_ID"
char ssid[] = "your_wifi_name";
char pass[] = "your_wifi_password";
```

5. Connect components according to the pin configuration below.

6. Upload the code to your ESP32.

7. Open Blynk dashboard and configure widgets based on the mapping table.

**🧾 Example Output (Serial Monitor)**

PIR: 1 | LDR: 2873 | Counter: 3 | Power (W): 2.407

PIR: 0 | LDR: 3120 | Counter: 3 | Power (W): 0.0


**⚙️ Hardware Used**
- ESP32 (DevKit V1)
- PIR Sensor (HC-SR501 or equivalent)
- LDR (Light Dependent Resistor)
- LED
- Stepper Motor (4-wire)
- Breadboard, resistors, jumper wires, power source

| Component     | Pin            | Description                       |
| ------------- | -------------- | --------------------------------- |
| PIR Sensor    | 12             | Motion detection                  |
| LDR           | 34             | Analog input for brightness       |
| LED           | 14             | PWM output for brightness control |
| Stepper Motor | 33, 25, 26, 27 | IN1–IN4 control pins              |

**🧠 How It Works**

1. The PIR sensor detects motion.
2. When motion is detected, the LED turns on and adjusts its brightness according to the LDR value.
3. The stepper motor rotates to simulate an energy-related action.
4. After 10 seconds of no motion, the LED and motor turn off automatically.
5. The ESP32 calculates instantaneous power usage and sends data to Blynk via Wi-Fi.
6. Telegram bot issues alerts when motion is detected.
7. Blynk switches allow the user to manually enable or disable LED and motor operation.


**📡 Blynk Dashboard**
| Virtual Pin | Data Name        | Widget Type           | Description                                      |
| ----------- | ---------------- | --------------------- | ------------------------------------------------ |
| **V1**      | Occupancy        | Gauge / Value Display | Shows motion detection (1 = Occupied, 0 = Empty) |
| **V2**      | LDR Reading      | Gauge                 | Displays ambient light intensity (0–4095)        |
| **V3**      | Hourly Counter   | Graph                 | Tracks the number of occupancy events per hour   |
| **V4**      | Electricity Used | Graph                 | Displays real-time power usage in watts          |
| **V5**    	| LED Override     | Switch                | Manual LED control                               |
| **V6**      |	Motor Override   | Switch                | Manual fan control                               |

Dashboard Overview:
- Occupancy Gauge: Indicates whether motion is currently detected.
- LDR Reading: Reflects ambient light level to control LED brightness automatically.
- Electricity Used Graph: Plots real-time power consumption.
- Hourly Counter Graph: Shows cumulative occupancy events over time.

<img width="1458" height="958" alt="image" src="https://github.com/user-attachments/assets/6ab3a7de-f818-4480-a3ac-a573cad3a136" />


**📜 License**

This project is open-source and available under the MIT License.
You are free to use, modify, and improve it for personal or educational purposes.
