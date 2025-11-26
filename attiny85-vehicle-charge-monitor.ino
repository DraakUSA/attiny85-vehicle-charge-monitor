// --- Pin Definitions (using the Arduino pin numbers for ATtiny85) ---
const int VOLTAGE_IN_PIN = A1; // Corresponds to ATtiny85 Pin PB2
const int LED1_RED_PIN = 0;    // Corresponds to ATtiny85 Pin PB0 (PWM) - CHARGING LED
const int LED1_GRN_PIN = 1;    // Corresponds to ATtiny85 Pin PB1 (PWM)
const int LED2_RED_PIN = 3;    // Corresponds to ATtiny85 Pin PB3 (Digital/A3) - BATTERY LED
const int LED2_GRN_PIN = 4;    // Corresponds to ATtiny85 Pin PB4 (PWM)

// --- Calibration and Thresholds ---
// R_RATIO is 0.25 for a 20V max input mapping to 5V max output (R1=15k, R2=5k)
const float R_RATIO = 0.25;
const float ADC_MAX = 1023.0;

// Voltage thresholds (in Volts) based on Gammatronix 12V Modes
const float V_OVER_CHARGE = 15.2; 
const float V_CHARGING_OK = 13.2; // Separates LED 1 (Charging) from LED 2 (Battery)
const float V_BATT_GREEN  = 12.1; 
const float V_YELLOW      = 11.8; 
const float V_YELLOW_FLASH= 11.5; 
const float V_RED_SOLID   = 11.2; 
const float V_RED_FLASH   = 11.0; 
const float V_RED_FAST_FLASH= 10.7; 

// --- Flashing Durations (in milliseconds) ---
const int SLOW_FLASH_RATE = 500; // 1000ms period (500ms ON / 500ms OFF)
const int FAST_FLASH_RATE = 200; // 400ms period (200ms ON / 200ms OFF)

void setup() {
  // Set all LED pins as outputs
  pinMode(LED1_RED_PIN, OUTPUT);
  pinMode(LED1_GRN_PIN, OUTPUT);
  pinMode(LED2_RED_PIN, OUTPUT);
  pinMode(LED2_GRN_PIN, OUTPUT);
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

void loop() {
  // Read the analog voltage
  int adcValue = analogRead(VOLTAGE_IN_PIN);
  
  // Calculate the actual voltage (V_Ref is 5.0V)
  float voltage = (float)adcValue / ADC_MAX * 5.0 / R_RATIO;
  
  // --- Reset both LEDs (Critical for flashing to work) ---
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW); // Charging LED OFF
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW); // Battery LED OFF
  
  
  // ====================================================================
  // 1. CHARGING MODE (MODE 2): Active when voltage is high (LED 1)
  // ====================================================================
  if (voltage >= V_CHARGING_OK) {
    
    if (voltage > V_OVER_CHARGE) {
      // Over-Voltage (> 15.2V): Alternating Red and Green Flash
      if (isFastFlash()) {
        // First half of the cycle (200ms): Red ON, Green OFF
        setLED(LED1_RED_PIN, LED1_GRN_PIN, HIGH, LOW); 
      } else {
        // Second half of the cycle (200ms): Red OFF, Green ON
        setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH); 
      }
      
    } else { // voltage is between V_CHARGING_OK and V_OVER_CHARGE
      // Normal Charging (13.2V - 15.2V): Solid Green
      setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, HIGH); 
    }
    
  } 
  
  // ====================================================================
  // 2. BATTERY MONITOR MODE (MODE 1): Active when voltage is low (LED 2)
  // This 'else' branch covers all voltages BELOW V_CHARGING_OK (13.2V)
  // ====================================================================
  else {
    
    if (voltage >= V_BATT_GREEN) {
      // Healthy Battery (12.1V - 13.2V): Solid Green
      setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, HIGH); 
      
    } else if (voltage >= V_YELLOW) {
      // Yellow (11.8V - 12.1V): Solid Yellow/Orange
      setLED(LED2_RED_PIN, LED2_GRN_PIN, 255, 100); 
      
    } else if (voltage >= V_YELLOW_FLASH) {
      // Yellow Flash (11.5V - 11.8V): Flashing Yellow/Orange
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, 255, 100); 
      }
      
    } else if (voltage >= V_RED_SOLID) {
      // Red (11.2V - 11.5V): Solid Red
      setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      
    } else if (voltage >= V_RED_FLASH) {
      // Red Flash (11.0V - 11.2V): Slow Flashing Red
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }
      
    } else if (voltage >= V_RED_FAST_FLASH) {
      // Red Fast Flash (10.7V - 11.0V): Fast Flashing Red
      if (isFastFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }
      
    } else {
      // Critical Low (< 10.7V): Solid Red
      setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
    }
  }

  delay(100); // Short delay for continuous monitoring
}
