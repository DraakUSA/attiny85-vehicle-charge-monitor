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
const int LED1_RED_PIN = 0;    // CHARGING LED
const int LED1_GRN_PIN = 1;
const int LED2_RED_PIN = 3;    // BATTERY LED
const int LED2_GRN_PIN = 4;

// --- Calibration and Thresholds ---
const float R_RATIO = 0.25;
const float ADC_MAX = 1023.0;

// Voltage thresholds (in Volts) - These are based on the original Gammatronix map
// Charging voltages
const float V_OVER_CHARGE = 15.0;
const float V_HIGH_CHARGE = 14.8;
const float V_CHARGING_OK = 13.5;
const float V_LOW_CHARGE  = 12.8;
// Battery voltages
const float V_BATT_GREEN  = 12.1;
const float V_YELLOW      = 11.8;
const float V_YELLOW_FLASH= 11.5;
const float V_ALT_YEL_RED = 11.2;    // Alternating Yellow/Red flash
const float V_RED_SOLID   = 11.0;    // Solid Red
const float V_RED_FLASH   = 10.7;    // Slow Red flash

// --- Battery LED Mode Selection ---
// Set to 1 for MODE 1 (original: Yellow/Red thresholds)
// Set to 5 for MODE 5 (Gammatronix: Green flash thresholds)
const int LED2_MODE = 1;  // Change to 5 for MODE 5 behavior

// --- MODE 5 Green Thresholds (Gammatronix 6-mode system) ---
// MODE 5 mapping:
//  - >= 12.8V : Solid Green
//  - >= 12.5V : Slow Green Flash
//  - >= 12.1V : Fast Green Flash
const float V_MODE5_GREEN_SOLID = 12.8;       // Solid green at/above 12.8V
const float V_MODE5_GREEN_SLOW_FLASH = 12.5;  // Slow green flash from 12.5V up to <12.8V
const float V_MODE5_GREEN_FAST_FLASH = 12.1;  // Fast green flash from 12.1V up to <12.5V

// --- Flashing Durations (in milliseconds) ---
const int SLOW_FLASH_RATE = 500;
const int FAST_FLASH_RATE = 200;

// --- Color Constants ---
const int YELLOW_R = 255;
const int YELLOW_G = 100;

// ====================================================================
// --- AVERAGING FILTER VARIABLES ---
// ====================================================================
const int WINDOW_SIZE = 10;
int readings[WINDOW_SIZE]; // The array to store the last N readings
long total = 0;            // The sum of all readings in the array
int readIndex = 0;         // The index of the current reading (where the next one goes)
// ====================================================================


void setup() {
  // Set all LED pins as outputs
  pinMode(LED1_RED_PIN, OUTPUT);
  pinMode(LED1_GRN_PIN, OUTPUT);
  pinMode(LED2_RED_PIN, OUTPUT);
  pinMode(LED2_GRN_PIN, OUTPUT);

  // --- PRE-FILL THE AVERAGING ARRAY ---
  // 1. Take a clean initial reading
  int initialReading = analogRead(VOLTAGE_IN_PIN);

  // 2. Populate every slot with the initial reading and set the total
  for (int i = 0; i < WINDOW_SIZE; i++) {
    readings[i] = initialReading;
    total += initialReading;
  }

  // Run LED test sequence once at startup
  ledTest();
}

// Helper function to set LED color
void setLED(int redPin, int greenPin, int redVal, int greenVal) {
  digitalWrite(redPin, redVal);
  digitalWrite(greenPin, greenVal);
}

// Helper function for slow flashing
bool isSlowFlash() {
  return (millis() % SLOW_FLASH_RATE) < (SLOW_FLASH_RATE / 2);
}

// Helper function for fast flashing
bool isFastFlash() {
  return (millis() % FAST_FLASH_RATE) < (FAST_FLASH_RATE / 2);
}

// Run a brief LED test sequence: LED1 then LED2, then flash both yellow twice
void ledTest() {
  const int TEST_DELAY = 300; // ms per step

  // Ensure all off
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);
  delay(TEST_DELAY);

  // LED1: Red, Green, Yellow
  setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, LOW);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, HIGH);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
  delay(TEST_DELAY / 2);

  // LED2: Red, Green, Yellow
  setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, HIGH);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);
  delay(TEST_DELAY / 2);

  // Flash both yellow twice (on/off cycles)
  for (int i = 0; i < 2; i++) {
    setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, HIGH);
    setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, HIGH);
    delay(TEST_DELAY);
    setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
    setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);
    delay(TEST_DELAY);
  }

  // Ensure all off at end
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);
  delay(TEST_DELAY / 2);
}

void loop() {
  // ====================================================================
  // 1. RUNNING AVERAGE CALCULATION
  // ====================================================================
  // Subtract the oldest reading from the total
  total = total - readings[readIndex];

  // Read the new ADC value
  int newReading = analogRead(VOLTAGE_IN_PIN);

  // Store the new reading in the array
  readings[readIndex] = newReading;

  // Add the new reading to the total
  total = total + newReading;

  // Advance the index (and wrap around when reaching the end of the array)
  readIndex = (readIndex + 1) % WINDOW_SIZE;

  // Calculate the average ADC value
  float averageADC = (float)total / WINDOW_SIZE;

  // Convert the average ADC value to the actual voltage
  float averageVoltage = averageADC / ADC_MAX * 5.0 / R_RATIO;
  // ====================================================================

  // --- Reset both LEDs (Critical for flashing to work) ---
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW);


  // ====================================================================
  // 2. CHARGING MODE (MODE 2): Active when voltage is high (LED 1)
  // Logic uses the stable 'averageVoltage'
  // ====================================================================
  if (averageVoltage >= V_CHARGING_OK) {

    if (averageVoltage > V_OVER_CHARGE) {
      // Over-Voltage (> 15.0V): Alternating Red and Green Flash
      if (isFastFlash()) {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, LOW);
      } else {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
      }

    } else if (averageVoltage > V_HIGH_CHARGE) {
      // High Charging (14.8V - 15.0V): Alternating Yellow/Green Flash
      if (isSlowFlash()) {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, HIGH);
      } else {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
      }

    } else {
      // Normal Charging (13.5V - 14.8V): Solid Green
      setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
    }

  }

  // ====================================================================
  // 3. BATTERY MONITOR MODE: Active when voltage is low (LED 2)
  // Logic uses the stable 'averageVoltage'
  // Selectable between MODE 1 (Solid green) and MODE 5 (Detailed green flash)
  // ====================================================================
  else {

    // --- LOW CHARGE INDICATION (LED 1) ---
    if (averageVoltage >= V_LOW_CHARGE) {
      if (isSlowFlash()) {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
      }
    }

    // Battery Voltage Indication (LED 2)
    if (averageVoltage >= V_BATT_GREEN) {
      if (LED2_MODE == 5) {
        // --- MODE 5: Gammatronix Green Flashing System ---
        if (averageVoltage >= V_MODE5_GREEN_SOLID) {
          // Solid Green (>= 12.8V)
          setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);

        } else if (averageVoltage >= V_MODE5_GREEN_SLOW_FLASH) {
          // Slow Flashing Green (12.5V - <12.8V)
          if (isSlowFlash()) {
            setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);
          }

        } else {
          // Fast Flashing Green (12.1V - <12.5V)
          if (isFastFlash()) {
            setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);
          }
        }

      } else {
        // Healthy Battery (12.1V - 13.5V): Solid Green
        setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);
      }

    } else if (averageVoltage >= V_YELLOW) {
      // Yellow (11.8V - 12.1V): Solid Yellow/Orange
      setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, HIGH);

    } else if (averageVoltage >= V_YELLOW_FLASH) {
      // Yellow Flash (11.5V - 11.8V): Flashing Yellow/Orange
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, HIGH);
      }

    } else if (averageVoltage >= V_ALT_YEL_RED) {
      // Alternating Yellow/Red Flash (11.2V - 11.5V)
      if (isFastFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      } else {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, HIGH);
      }

    } else if (averageVoltage >= V_RED_SOLID) {
      // Solid Red (11.0V - 11.2V)
      setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);

    } else if (averageVoltage >= V_RED_FLASH) {
      // Slow Flashing Red (10.7V - 11.0V)
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }

    } else {
      // Fast Flashing Red (< 10.7V)
      if (isFastFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }
    }
  }

  delay(100); // Short delay for continuous monitoring
}
