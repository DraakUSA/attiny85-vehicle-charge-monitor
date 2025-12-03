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

## 🎯 Updated Operational Logic and Thresholds

The code implements a parallel logic, allowing both LEDs to indicate status simultaneously:

* **LED 1 (Charging Indicator) is Active when:** The system is monitoring alternator output, which is typically any time the **engine is running** (or when $\mathbf{V > 12.1V}$, confirming the battery is not dead).
* **LED 2 (Battery Monitor) is Active when:** The system is monitoring the battery's state of charge (SoC), which is typically when the **engine is off** or when **voltage drops below the optimal charging threshold ($\mathbf{13.5V}$)**.

### 🔌 Indicator Activation Logic

| Indicator | Activation Condition | Rationale |
| :--- | :--- | :--- |
| **LED 1 (Charging Status)** | $\mathbf{V > 12.1V}$ **AND Engine Running** | Active only when the engine is running to monitor the alternator's true output status. Ignored otherwise. |
| **LED 2 (Battery Monitor)** | **$\mathbf{V < 13.5V}$** | Active when the engine is off or when the system is not fully charging. **Turns OFF when $\mathbf{V \ge 13.5V}$** (to signal the alternator is fully handling power). |

### ⚠️ Overlap and Interpretation

Since both LEDs can be active simultaneously, the user must interpret the two lights together:

* **Scenario 1 (Good):** **LED 1 is Solid Green** ($13.5\text{V} \to 14.8\text{V}$) AND **LED 2 is OFF**.
    * 👉 **System is healthy and charging is optimal.** The alternator is performing correctly, and the battery monitor is inactive.
* **Scenario 2 (Weak):** **LED 1 is Solid Yellow** ($12.8\text{V} \to 13.5\text{V}$) AND **LED 2 is Solid Green**.
    * 👉 **Alternator is weak/undercharging.** The voltage is too low for optimal charge, but the battery is still healthy.
* **Scenario 3 (Failure):** **LED 1 is Solid Red** ($12.1\text{V} \to 12.8\text{V}$) AND **LED 2 is Solid/Flashing Green**.
    * 👉 **Alternator has failed.** The system is running entirely on the battery, but the battery is still above the critical low charge threshold.
* **Scenario 4 (Critical):** **LED 1 is OFF** ($\le 12.1\text{V}$) AND **LED 2 is Solid/Flashing Yellow/Red**.
    * 👉 **Alternator failed AND the battery is critically discharged.** Battery is below $50\%\text{ SoC}$; there is risk of permanent damage and system failure.
* **Scenario 5 (High-Charge):** **LED 1 is Alternating Yellow/Green** ($14.8\text{V} \to 15.0\text{V}$) AND **LED 2 is OFF**.
    * 👉 **Regulator Warning.** Alternator output is elevated and near the safe limit. **Monitor closely; high voltage detected.**
* **Scenario 6 (Over-Voltage):** **LED 1 is Alternating Red/Green Flash (Fast)** ($> 15.0\text{V}$) AND **LED 2 is OFF**.
    * 👉 **CRITICAL FAILURE.** The voltage regulator has failed, posing a **risk of immediate battery and electronics damage.** Stop driving immediately.

**Configuration Notes:**

* Voltage thresholds are calibrated based on Gammatronix reference behavior but customized for this implementation.
* LED 2 can operate in simple (**MODE 1**) or detailed (**MODE 5**). Set `LED2_MODE` constant in the code to select.
* All threshold values are user-adjustable in the source code for fine-tuning to specific vehicle requirements.

| Voltage Range (V) | Active LED | State (LED Action) | Meaning |
| :--- | :--- | :--- | :--- |
| <td colspan="4" align="center">**Charging Status Indicator**</td> |
| **$> 15.0V$** | **LED 1** | Alternating R/G Flash (Fast) | **Over-Voltage Warning / Alternator Fault** |
| **$14.8V \to 15.0V$** | **LED 1** | Alternating Yellow/Green (Slow) | High-charge band — elevated alternator output |
| **$13.5V \to 14.8V$** | **LED 1** | Solid Green | Normal Charging (Alternator OK) |
| **$12.8V \to 13.5V$** | **LED 1** | Solid Yellow/Orange | Low-charge indication (Alternator Weak) |
| **$12.1V \to 12.8V$** | **LED 1** | Solid Red | **Alternator Failure / Running on Battery Power** |
| <td colspan="4" align="center">**Battery Status Indicator**</td> |
| **$12.7V \to 13.5V$** | **LED 2** | Solid Green | **Battery Fully Charged / Optimal Resting** |
| **$12.4V \to 12.7V$** | **LED 2** | Slow Green Flash (MODE 5) / Solid Green (MODE 1) | **High Charge** (Standard 100% resting voltage) - MODE 5: slow green flash band; MODE 1: solid green (12.1V+) |
| **$12.1V \to 12.4V$** | **LED 2** | Fast Green Flash (MODE 5) / Solid Green (MODE 1) | **Acceptable Charge** (Still above 75% SoC) - MODE 5: fast green flash; MODE 1: solid green |
| **$11.8V \to 12.1V$** | **LED 2** | Solid Yellow/Orange | **LED 1 Turns OFF Here.** Battery Discharge Warning (Mode 1 Yellow) |
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
const float V_LOW_CHARGE  = 12.8;
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
