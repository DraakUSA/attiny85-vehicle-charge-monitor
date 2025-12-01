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
const float V_OVER_CHARGE = 15.2;
const float V_CHARGING_OK = 13.2;
const float V_BATT_GREEN  = 12.1;
const float V_YELLOW      = 11.8;
const float V_YELLOW_FLASH= 11.5;
const float V_ALT_YEL_RED = 11.2;    // Alternating Yellow/Red flash
const float V_RED_SOLID   = 11.0;    // Solid Red
const float V_RED_FLASH   = 10.7;    // Slow Red flash

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
  analogWrite(redPin, redVal);
  analogWrite(greenPin, greenVal);
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
  setLED(LED1_RED_PIN, LED1_GRN_PIN, 0, 0);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, 0, 0);
  delay(TEST_DELAY);

  // LED1: Red, Green, Yellow
  setLED(LED1_RED_PIN, LED1_GRN_PIN, 255, 0);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, 0, 255);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, YELLOW_R, YELLOW_G);
  delay(TEST_DELAY);
  setLED(LED1_RED_PIN, LED1_GRN_PIN, 0, 0);
  delay(TEST_DELAY / 2);

  // LED2: Red, Green, Yellow
  setLED(LED2_RED_PIN, LED2_GRN_PIN, 255, 0);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, 0, 255);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, YELLOW_R, YELLOW_G);
  delay(TEST_DELAY);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, 0, 0);
  delay(TEST_DELAY / 2);

  // Flash both yellow twice (on/off cycles)
  for (int i = 0; i < 2; i++) {
    setLED(LED1_RED_PIN, LED1_GRN_PIN, YELLOW_R, YELLOW_G);
    setLED(LED2_RED_PIN, LED2_GRN_PIN, YELLOW_R, YELLOW_G);
    delay(TEST_DELAY);
    setLED(LED1_RED_PIN, LED1_GRN_PIN, 0, 0);
    setLED(LED2_RED_PIN, LED2_GRN_PIN, 0, 0);
    delay(TEST_DELAY);
  }

  // Ensure all off at end
  setLED(LED1_RED_PIN, LED1_GRN_PIN, 0, 0);
  setLED(LED2_RED_PIN, LED2_GRN_PIN, 0, 0);
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
      // Over-Voltage (> 15.2V): Alternating Red and Green Flash
      if (isFastFlash()) {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, LOW);
      } else {
        setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
      }

    } else {
      // Normal Charging (13.2V - 15.2V): Solid Green
      setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH);
    }

  }

  // ====================================================================
  // 3. BATTERY MONITOR MODE (MODE 1): Active when voltage is low (LED 2)
  // Logic uses the stable 'averageVoltage'
  // ====================================================================
  else {

    if (averageVoltage >= V_BATT_GREEN) {
      // Healthy Battery (12.1V - 13.2V): Solid Green
      setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH);

    } else if (averageVoltage >= V_YELLOW) {
      // Yellow (11.8V - 12.1V): Solid Yellow/Orange
      setLED(LED2_RED_PIN, LED2_GRN_PIN, YELLOW_R, YELLOW_G);

    } else if (averageVoltage >= V_YELLOW_FLASH) {
      // Yellow Flash (11.5V - 11.8V): Flashing Yellow/Orange
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, YELLOW_R, YELLOW_G);
      }

    } else if (averageVoltage >= V_ALT_YEL_RED) {
      // Alternating Yellow/Red Flash (11.2V - 11.5V)
      if (isFastFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      } else {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, YELLOW_R, YELLOW_G);
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
