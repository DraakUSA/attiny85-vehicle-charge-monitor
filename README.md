# 🚗 ATtiny85 Vehicle Voltage Monitor (Gammatronix Emulation)

A compact, ATtiny85-based voltage monitoring circuit designed to monitor a vehicle's 12V charging system. It emulates the behavior of the Gammatronix "J" monitor by using two bi-color LEDs to provide mutually exclusive feedback on **Charging Status (LED 1)** and **Battery Health (LED 2)**.

The circuit uses a voltage divider calibrated for safe input up to 20V.

---

## ✨ Features

* **Dual Mode Display:** Clearly separates the Charging State (Mode 2) from the Battery State (Mode 1).
* **Voltage Divider:** Calibrated for safe measurement up to 20V (using 15kΩ and 5kΩ resistors).
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
| **R1 (Voltage Divider)** | **15 k$\Omega$** |
| **R2 (Voltage Divider)** | **5 k$\Omega$** |
| Current Limit Resistors | 4 x **220 $\Omega$** (for LEDs) |

### 2. Pinout and Connections

The analog input is calibrated for a **VCC = 5.0V** reference. The code uses the Arduino IDE pin numbers for the ATtiny85 core.

| Function | ATtiny Pin | Arduino Pin No. | Connection |
| :--- | :--- | :--- | :--- |
| Voltage Input | **PB2** | **A1 (Pin 7)** | Output of the 15k$\Omega$ / 5k$\Omega$ voltage divider. |
| LED 1 (Charge) Red | **PB0** | **0 (Pin 5)** | To Red pin of LED 1 (via $220\Omega$ resistor). |
| LED 1 (Charge) Green | **PB1** | **1 (Pin 6)** | To Green pin of LED 1 (via $220\Omega$ resistor). |
| LED 2 (Battery) Red | **PB3** | **3 (Pin 2)** | To Red pin of LED 2 (via $220\Omega$ resistor). |
| LED 2 (Battery) Green | **PB4** | **4 (Pin 3)** | To Green pin of LED 2 (via $220\Omega$ resistor). |
| Ground | GND | GND | Connects to Vehicle GND, $R_2$, and LED Cathodes. |

---

## 🎯 Operational Logic and Thresholds

The code implements a mutually exclusive logic based on the $13.2V$ charging threshold:

* **LED 1 (Charging Indicator)** is active when $\mathbf{Voltage \ge 13.2V}$ (Mode 2).
* **LED 2 (Battery Monitor)** is active when $\mathbf{Voltage < 13.2V}$ (Mode 1).

| Voltage Range (V) | Active LED | State | Meaning (Gammatronix Emulation) |
| :--- | :--- | :--- | :--- |
| **$> 15.2V$** | **LED 1** | **Alternating R/G Flash** (Fast) | **Over-Voltage Warning / Alternator Fault** |
| **$13.2V \to 15.2V$** | **LED 1** | Solid Green | Normal Charging (Alternator OK) |
| **$12.1V \to 13.2V$** | **LED 2** | Solid Green | Healthy Battery (Engine OFF / Standby) |
| **$11.8V \to 12.1V$** | **LED 2** | Solid Yellow/Orange | Battery Discharge Warning (Mode 1 Yellow) |
| **$11.5V \to 11.8V$** | **LED 2** | Yellow/Orange Flash (Slow) | Low Battery Capacity Warning |
| **$11.2V \to 11.5V$** | **LED 2** | Solid Red | Critical Low Voltage |
| **$11.0V \to 11.2V$** | **LED 2** | Red Flash (Slow) | Battery Discharged / Near Dead |
| **$10.7V \to 11.0V$** | **LED 2** | Red Flash (Fast) | **Emergency Stop** (Dangerously low voltage) |
| **$< 10.7V$** | **LED 2** | Solid Red | System voltage too low to operate reliably. |

---

## 💻 Arduino Code (Core Logic)

The code uses the `analogRead()` and `analogWrite()` functions along with the `millis()` function to achieve non-blocking voltage reading and flashing states.

```cpp
// --- Voltage thresholds (in Volts) ---
const float V_OVER_CHARGE = 15.2; 
const float V_CHARGING_OK = 13.2; 
const float V_BATT_GREEN  = 12.1; 
const float V_YELLOW      = 11.8; 
// ... (All other constants defined in the full code) ...

void loop() {
  // 1. Calculate Voltage
  float voltage = (float)analogRead(A1) / 1023.0 * 5.0 / 0.25;
  
  // 2. Reset LEDs (Critical for flashing states)
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW); 
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW); 
  
  // 3. Mutually Exclusive Logic
  if (voltage >= V_CHARGING_OK) {
    // LED 1 (Charging Mode) Logic runs here...
    // Includes Alternating R/G Flash for V > 15.2V
  } else {
    // LED 2 (Battery Mode) Logic runs here...
    // Includes all Green, Yellow, and Red/Flashing states below 13.2V
  }

  delay(100); 
}
