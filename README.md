# 🚗 ATtiny85 Vehicle Voltage Monitor (Gammatronix Emulation)

A compact, ATtiny85-based voltage monitoring circuit designed to monitor a vehicle's 12V charging system. It emulates the behavior of the Gammatronix "J" monitor by using two bi-color LEDs to provide mutually exclusive feedback on **Charging Status (LED 1)** and **Battery Health (LED 2)**.

The circuit uses a voltage divider calibrated for safe input up to 20V.

> ⚠️ **Note:** The voltage thresholds and LED indications are based on Gammatronix reference values but are **not exact replicas**. They are calibrated for typical automotive use and may differ from the original Gammatronix monitor.

See the original Gammatronix "J" monitor product page: [Gammatronix "J" monitor](https://gammatronixltd.com/epages/bae94c71-c5b6-4572-89a1-e89006e78fbe.sf/en_GB/?ObjectPath=/Shops/bae94c71-c5b6-4572-89a1-e89006e78fbe/Products/J)

---

## ✨ Features

* **Dual Mode Display:** Clearly separates the Charging State (Mode 2) from the Battery State (Mode 1).
* **Voltage Divider:** Calibrated for safe measurement up to 20V (using 30k and 10k resistors).
* **High-Alert Flashing:** Implements slow, fast, and alternating Red/Green flashing for critical alerts.
* **Low Power:** Ideal for permanent installation in a vehicle dashboard or control panel.

---

## 🛠️ Hardware & Components

### 1. Components

| Component | Specification |
| :--- | :--- |
| Microcontroller | **ATtiny85** |
| LEDs | 2 x **Bi-Color LEDs** (Red/Green, Common Cathode Recommended) |
| Voltage Regulator | **7805** or equivalent DC-DC buck converter |
| **R1 (Voltage Divider)** | **30k Ohm** |
| **R2 (Voltage Divider)** | **10k Ohm** |
| Current Limit Resistors | 4 x **220 Ohm** (for LEDs) |

### 2. Pinout and Connections

The analog input is calibrated for a **VCC = 5.0V** reference. The code uses the Arduino IDE pin numbers for the ATtiny85 core.

| Function | ATtiny Pin | Arduino Pin No. | Connection |
| :--- | :--- | :--- | :--- |
| Voltage Input | **PB2** | **A1 (Pin 7)** | Output of the 30k / 10k voltage divider. |
| LED 1 (Charge) Red | **PB0** | **0 (Pin 5)** | To Red pin of LED 1 (via 220 Ohm resistor). |
| LED 1 (Charge) Green | **PB1** | **1 (Pin 6)** | To Green pin of LED 1 (via 220 Ohm resistor). |
| LED 2 (Battery) Red | **PB3** | **3 (Pin 2)** | To Red pin of LED 2 (via 220 Ohm resistor). |
| LED 2 (Battery) Green | **PB4** | **4 (Pin 3)** | To Green pin of LED 2 (via 220 Ohm resistor). |
| Ground | GND | GND | Connects to Vehicle GND, R2, and LED Cathodes. |
| Reset / Programming | **PB5** | **5 (Pin 1)** | Reset line; connected to VCC via a 10k pull-up resistor; used for ISP/programming (do not hold low during normal operation). |

---

## 🎯 Operational Logic and Thresholds

The code implements a mutually exclusive logic based on the $13.5V$ charging threshold:

* **LED 1 (Charging Indicator)** is active when $\mathbf{Voltage \ge 13.5V}$ (Mode 2).
* **LED 2 (Battery Monitor)** is active when $\mathbf{Voltage < 13.5V}$ (Mode 1).

**Configuration Notes:**
- Voltage thresholds are calibrated based on Gammatronix reference behavior but customized for this implementation.
- LED2 can operate in **MODE 1** (default, yellow/red thresholds) or **MODE 5** (green flash thresholds). Set `LED2_MODE` constant in the code to select.
- All threshold values are user-adjustable in the source code for fine-tuning to specific vehicle requirements.

| Voltage Range (V) | Active LED | State (LED Action) | Meaning (Gammatronix Emulation) |
| :--- | :--- | :--- | :--- |
| **$> 15.0V$** | **LED 1** | Alternating R/G Flash (Fast) | **Over-Voltage Warning / Alternator Fault** |
| **$14.8V \to 15.0V$** | **LED 1** | Alternating Yellow/Green (Slow) | High-charge band — elevated alternator output |
| **$13.5V \to 14.8V$** | **LED 1** | Solid Green | Normal Charging (Alternator OK) |
| **$12.6V \to 13.5V$** | **LED 1** | Slow Green Flash | Low-charge indication (LED1 slow green flash when voltage is between `V_LOW_CHARGE` and the charging threshold; may appear alongside LED2 indications) |
| **$12.8V \to 13.5V$** | **LED 2** | Solid Green (MODE 5) / Solid Green (MODE 1 for 12.1V+) | Battery healthy (MODE 5 shows solid green in this top band) |
| **$12.5V \to 12.8V$** | **LED 2** | Slow Green Flash (MODE 5) | MODE 5: slow green flash band |
| **$12.1V \to 12.5V$** | **LED 2** | Fast Green Flash (MODE 5) / Solid Green (MODE 1) | MODE 5: fast green flash; MODE 1: solid green (12.1V+) |
| **$11.8V \to 12.1V$** | **LED 2** | Solid Yellow/Orange | Battery Discharge Warning (Mode 1 Yellow) |
| **$11.5V \to 11.8V$** | **LED 2** | Yellow/Orange Flash (Slow) | Low Battery Capacity Warning |
| **$11.2V \to 11.5V$** | **LED 2** | Alternating Yellow/Red Flash (Fast) | **Critical Low Voltage** |
| **$11.0V \to 11.2V$** | **LED 2** | Solid Red | Battery Discharged / Near Dead |
| **$10.7V \to 11.0V$** | **LED 2** | Red Flash (Slow) | Dangerously Low Battery |
| **$< 10.7V$** | **LED 2** | Red Flash (Fast) | **Emergency Stop / System Voltage Too Low** |

---

## 💻 Arduino Code (Core Logic)

The code uses the `analogRead()` and `analogWrite()` functions along with the `millis()` function to achieve non-blocking voltage reading and flashing states.

```cpp
// --- Voltage thresholds (in Volts) ---
const float V_OVER_CHARGE = 15.0;
const float V_HIGH_CHARGE = 14.8;
const float V_CHARGING_OK = 13.5;
const float V_LOW_CHARGE  = 12.6;
const float V_BATT_GREEN  = 12.1;
// MODE 5 green thresholds (if enabled via LED2_MODE):
// solid >= 12.8, slow flash >= 12.5, fast flash >= 12.1
const float V_MODE5_GREEN_SOLID = 12.8;
const float V_MODE5_GREEN_SLOW_FLASH = 12.5;
const float V_MODE5_GREEN_FAST_FLASH = 12.1;
const float V_YELLOW      = 11.8;
// ... (All other constants defined in the full code) ...

void loop() {
  // 1. Calculate Voltage
  // R_RATIO = 0.25 for 15k and 5k resistors
  float voltage = (float)analogRead(A1) / 1023.0 * 5.0 / 0.25;

  // 2. Reset LEDs (Critical for flashing states)
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);

  // 3. Mutually Exclusive Logic
  if (voltage >= V_CHARGING_OK) {
    // LED 1 (Charging Mode) Logic runs here...
  } else {
    // LED 2 (Battery Mode) Logic runs here...
  }

  delay(100);
}
