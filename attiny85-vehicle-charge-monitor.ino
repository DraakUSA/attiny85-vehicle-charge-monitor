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
const float V_ALT_YEL_RED = 11.2;    // Threshold for the new Alternating Yellow/Red flash
const float V_RED_SOLID   = 11.0;    // Threshold for the new Solid Red
const float V_RED_FLASH   = 10.7;    // Threshold for the new Slow Red flash
// < 10.7V is now the Fast Red Flash state

// --- Flashing Durations (in milliseconds) ---
const int SLOW_FLASH_RATE = 500; 
const int FAST_FLASH_RATE = 200; 

void setup() {
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
  int adcValue = analogRead(VOLTAGE_IN_PIN);
  float voltage = (float)adcValue / ADC_MAX * 5.0 / R_RATIO;
  
  // --- Reset both LEDs (Critical for flashing to work) ---
  setLED(LED1_RED_PIN, LED1_GRN_PIN, LOW, LOW); 
  setLED(LED2_RED_PIN, LED2_GRN_PIN, LOW, LOW); 
  
  
  // ====================================================================
  // 1. CHARGING MODE (MODE 2): Active when voltage is high (LED 1)
  // ====================================================================
  if (voltage >= V_CHARGING_OK) {
    
    if (voltage > V_OVER_CHARGE) {
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
  // 2. BATTERY MONITOR MODE (MODE 1): Active when voltage is low (LED 2)
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
      
    } else if (voltage >= V_ALT_YEL_RED) {
      // NEW STATE: Alternating Yellow/Red Flash (11.2V - 11.5V)
      if (isFastFlash()) {
        // Red ON, Yellow OFF (Red + Low Green)
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW); 
      } else {
        // Yellow ON (Red + Green/low)
        setLED(LED2_RED_PIN, LED2_GRN_PIN, 255, 100);
      }
      
    } else if (voltage >= V_RED_SOLID) {
      // NEW STATE: Solid Red (11.0V - 11.2V)
      setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      
    } else if (voltage >= V_RED_FLASH) {
      // NEW STATE: Slow Flashing Red (10.7V - 11.0V)
      if (isSlowFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }
      
    } else {
      // NEW STATE: Fast Flashing Red (< 10.7V)
      if (isFastFlash()) {
        setLED(LED2_RED_PIN, LED2_GRN_PIN, HIGH, LOW);
      }
    }
  }

  delay(100); 
}
