/*
 * attiny85-vehicle-charge-monitor.ino
 * Copyright (c) 2025 Paul-Joseph de Werk
 *
 * This file is part of the attiny85-vehicle-charge-monitor project.
 * Licensed under the GNU General Public License v3. See the
 * LICENSE file in the project root for details.
 *
 * Repository: https://github.com/DraakUSA/attiny85-vehicle-charge-monitor
 */

// --- Pin Definitions (using the Arduino pin numbers for ATtiny85) ---
const int VOLTAGE_IN_PIN = A1;
const int LED1_RED_PIN = 0;     // CHARGING LED
const int LED1_GRN_PIN = 1;
const int LED2_RED_PIN = 3;     // BATTERY LED
const int LED2_GRN_PIN = 4;

// --- COLOR CONSTANTS ---
// These will be used for readability in the main loop
const int OFF = 0;
const int RED = 1;
const int GREEN = 2;
const int YELLOW = 3;

// --- Calibration and Thresholds ---
const float R_RATIO = 0.25;
const float ADC_MAX = 1023.0;

// ====================================================================
// --- Voltage thresholds (in Volts) - These are based on the original Gammatronix map ---
// ====================================================================
// --- Battery LED Mode Selection ---
// Set to 1 for MODE 1 (original: Yellow/Red thresholds)
// Set to 5 for MODE 5 (Gammatronix: Green flash thresholds)
const int LED2_MODE = 5;  // Change to 5 for MODE 5 behavior

// --- MODE 2 Charging voltages ---
const float V_OVER_CHARGE = 15.0;
const float V_HIGH_CHARGE = 14.8;
const float V_CHARGING_OK = 13.5;
const float V_LOW_CHARGE  = 12.8;

// --- MODE 5 Green Thresholds (Gammatronix 6-mode system) ---
const float V_MODE5_GREEN_SOLID = 12.7;       // Solid green at/above 12.7V
const float V_MODE5_GREEN_SLOW_FLASH = 12.4;  // Slow green flash from 12.4V up to <12.7V
const float V_MODE5_GREEN_FAST_FLASH = 12.1;  // Fast green flash from 12.1V up to <12.4V

// --- MODE 1 Battery voltages ---
const float V_BATT_GREEN  = 12.1;
const float V_YELLOW      = 11.8;
const float V_YELLOW_FLASH= 11.5;
const float V_ALT_YEL_RED = 11.2;     // Alternating Yellow/Red flash
const float V_RED_SOLID   = 11.0;     // Solid Red
const float V_RED_FLASH   = 10.7;     // Slow Red flash

// --- Flashing Durations (in milliseconds) ---
const int SLOW_FLASH_RATE = 500;
const int FAST_FLASH_RATE = 200;

// ====================================================================
// --- AVERAGING FILTER VARIABLES ---
// ====================================================================
const int WINDOW_SIZE = 10;
int readings[WINDOW_SIZE]; // The array to store the last N readings
long total = 0;             // The sum of all readings in the array
int readIndex = 0;          // The index of the current reading (where the next one goes)
// ====================================================================


void setup() {
  // Set all LED pins as outputs
  pinMode(LED1_RED_PIN, OUTPUT);
  pinMode(LED1_GRN_PIN, OUTPUT);
  pinMode(LED2_RED_PIN, OUTPUT);
  pinMode(LED2_GRN_PIN, OUTPUT);

  // --- PRE-FILL THE AVERAGING ARRAY ---
  int initialReading = analogRead(VOLTAGE_IN_PIN);
  for (int i = 0; i < WINDOW_SIZE; i++) {
    readings[i] = initialReading;
    total += initialReading;
  }

  // Run LED test sequence once at startup
  ledTest();
}

/**
 * @brief Sets the color of a specific LED group (bi-color Red/Green LED).
 *
 * @param redPin The pin connected to the Red element of the LED.
 * @param greenPin The pin connected to the Green element of the LED.
 * @param color The desired color (OFF, RED, GREEN, or YELLOW).
 */
void setLED_Color(int redPin, int greenPin, int color) {
  switch (color) {
    case OFF:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
    case RED:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      break;
    case GREEN:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      break;
    case YELLOW:
      // Note: Setting both Red and Green to HIGH results in Yellow/Orange
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, HIGH);
      break;
    default:
      // Should not happen, but turn off to be safe
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
  }
}

// Convenience function for LED 1 (Charging Status)
void setChargingLED(int color) {
  setLED_Color(LED1_RED_PIN, LED1_GRN_PIN, color);
}

// Convenience function for LED 2 (Battery Status)
void setBatteryLED(int color) {
  setLED_Color(LED2_RED_PIN, LED2_GRN_PIN, color);
}


// Helper function for slow flashing (on for 50% of SLOW_FLASH_RATE)
bool isSlowFlash() {
  return (millis() % SLOW_FLASH_RATE) < (SLOW_FLASH_RATE / 2);
}

// Helper function for fast flashing (on for 50% of FAST_FLASH_RATE)
bool isFastFlash() {
  return (millis() % FAST_FLASH_RATE) < (FAST_FLASH_RATE / 2);
}

// Run a brief LED test sequence
void ledTest() {
  const int TEST_DELAY = 300; // ms per step

  // Ensure all off
  setChargingLED(OFF);
  setBatteryLED(OFF);
  delay(TEST_DELAY);

  // LED1: Red, Green, Yellow
  setChargingLED(RED);
  delay(TEST_DELAY);
  setChargingLED(GREEN);
  delay(TEST_DELAY);
  setChargingLED(YELLOW);
  delay(TEST_DELAY);
  setChargingLED(OFF);
  delay(TEST_DELAY / 2);

  // LED2: Red, Green, Yellow
  setBatteryLED(RED);
  delay(TEST_DELAY);
  setBatteryLED(GREEN);
  delay(TEST_DELAY);
  setBatteryLED(YELLOW);
  delay(TEST_DELAY);
  setBatteryLED(OFF);
  delay(TEST_DELAY / 2);

  // Flash both yellow twice (on/off cycles)
  for (int i = 0; i < 2; i++) {
    setChargingLED(YELLOW);
    setBatteryLED(YELLOW);
    delay(TEST_DELAY);

    setChargingLED(OFF);
    setBatteryLED(OFF);
    delay(TEST_DELAY);
  }

  // Ensure all off at end
  setChargingLED(OFF);
  setBatteryLED(OFF);
  delay(TEST_DELAY / 2);
}

void loop() {
  // ====================================================================
  // 1. RUNNING AVERAGE CALCULATION (Unchanged)
  // ====================================================================
  total = total - readings[readIndex];
  int newReading = analogRead(VOLTAGE_IN_PIN);
  readings[readIndex] = newReading;
  total = total + newReading;
  readIndex = (readIndex + 1) % WINDOW_SIZE;

  float averageADC = (float)total / WINDOW_SIZE;
  float averageVoltage = averageADC / ADC_MAX * 5.0 / R_RATIO;
  // ====================================================================

  // --- Reset both LEDs (Critical for flashing to work) ---
  setChargingLED(OFF);
  setBatteryLED(OFF);


  // ====================================================================
  // 2. CHARGING MODE (LED 1)
  // ====================================================================
  if (averageVoltage >= V_CHARGING_OK) {

    if (averageVoltage > V_OVER_CHARGE) {
      // Over-Voltage (> 15.0V): Alternating Red and Green Flash
      if (isFastFlash()) {
        setChargingLED(RED);
      } else {
        setChargingLED(GREEN);
      }

    } else if (averageVoltage > V_HIGH_CHARGE) {
      // High Charging (14.8V - 15.0V): Alternating Yellow/Green Flash
      if (isSlowFlash()) {
        setChargingLED(YELLOW);
      } else {
        setChargingLED(GREEN);
      }

    } else {
      // Normal Charging (13.5V - 14.8V): Solid Green
      setChargingLED(GREEN);
    }

  }

  // ====================================================================
  // 3. BATTERY MONITOR MODE (LED 2)
  // ====================================================================
  else {

    // --- LOW CHARGE INDICATION (LED 1) ---
    if (averageVoltage >= V_LOW_CHARGE) {
      // Weak/Under Charge (12.8V - 13.5V): Yellow
      setChargingLED(YELLOW);
    } else if (averageVoltage >= V_BATT_GREEN) {
      // Severe/Failure (12.1V - <12.8V): Red
      setChargingLED(RED);
    }

    // Battery Voltage Indication (LED 2)
    if (averageVoltage >= V_BATT_GREEN) {
      if (LED2_MODE == 5) {
        // --- MODE 5: Gammatronix Green Flashing System ---
        if (averageVoltage >= V_MODE5_GREEN_SOLID) {
          // Solid Green (>= 12.7V)
          setBatteryLED(GREEN);

        } else if (averageVoltage >= V_MODE5_GREEN_SLOW_FLASH) {
          // Slow Flashing Green (12.4V - <12.7V)
          if (isSlowFlash()) {
            setBatteryLED(GREEN);
          }

        } else {
          // Fast Flashing Green (12.1V - <12.4V)
          if (isFastFlash()) {
            setBatteryLED(GREEN);
          }
        }

      } else {
        // Healthy Battery (12.1V - 13.5V): Solid Green
        setBatteryLED(GREEN);
      }

    } else if (averageVoltage >= V_YELLOW) {
      // Yellow (11.8V - 12.1V): Solid Yellow/Orange
      setBatteryLED(YELLOW);

    } else if (averageVoltage >= V_YELLOW_FLASH) {
      // Yellow Flash (11.5V - 11.8V): Flashing Yellow/Orange
      if (isSlowFlash()) {
        setBatteryLED(YELLOW);
      }

    } else if (averageVoltage >= V_ALT_YEL_RED) {
      // Alternating Yellow/Red Flash (11.2V - 11.5V)
      if (isFastFlash()) {
        setBatteryLED(RED); // Red ON
      } else {
        setBatteryLED(YELLOW); // Yellow ON
      }

    } else if (averageVoltage >= V_RED_SOLID) {
      // Solid Red (11.0V - 11.2V)
      setBatteryLED(RED);

    } else if (averageVoltage >= V_RED_FLASH) {
      // Slow Flashing Red (10.7V - 11.0V)
      if (isSlowFlash()) {
        setBatteryLED(RED);
      }

    } else {
      // Fast Flashing Red (< 10.7V)
      if (isFastFlash()) {
        setBatteryLED(RED);
      }
    }
  }

  delay(100); // Short delay for continuous monitoring
}
