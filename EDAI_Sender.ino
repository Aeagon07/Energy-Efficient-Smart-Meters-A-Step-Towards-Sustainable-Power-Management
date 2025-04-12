#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Pin and hardware configurations
#define CURRENT_SENSOR_PIN A0
#define BURDEN_RESISTOR 0.033
#define VOLTAGE_REFERENCE 5.0
#define CALIBRATION_FACTOR 0.6
#define SUPPLY_VOLTAGE 230.0

#define LORA_SS_PIN 10
#define LORA_RST_PIN 9
#define LORA_DIO0_PIN 2
#define LORA_BAND 433E6

#define EEPROM_ADDRESS 0
#define ENERGY_THRESHOLD 0.01 // Threshold for EEPROM update

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD with I2C address 0x27

float cumulativeEnergy = 0.0;
float previousCumulativeEnergy = 0.0;

// Function to read current from the sensor
float getCurrent() {
  int rawValue = analogRead(CURRENT_SENSOR_PIN);
  float voltage = (rawValue * VOLTAGE_REFERENCE) / 1023.0;
  float currentInAmperes = (voltage / BURDEN_RESISTOR) * CALIBRATION_FACTOR;
  return currentInAmperes * 1000; // Convert to mA
}

// EEPROM utility functions
void EEPROM_writeFloat(int address, float value) {
  byte* data = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, data[i]);
  }
}

float EEPROM_readFloat(int address) {
  float value = 0.0;
  byte* data = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    data[i] = EEPROM.read(address + i);
  }
  return value;
}

void initializeEEPROM() {
  float initialValue = 0.0;
  EEPROM_writeFloat(EEPROM_ADDRESS, initialValue);
  Serial.println("EEPROM initialized with default value: 0.0");
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  // Read cumulative energy from EEPROM
  cumulativeEnergy = EEPROM_readFloat(EEPROM_ADDRESS);

  // Check for invalid or out-of-range values
  if (isnan(cumulativeEnergy) || cumulativeEnergy < 0.0 || cumulativeEnergy > 10000.0) {
    cumulativeEnergy = 0.0;
    initializeEEPROM();
    Serial.println("Invalid EEPROM value detected. Reset to 0.0.");
  }

  Serial.print("Cumulative Energy Initialized: ");
  Serial.println(cumulativeEnergy, 4);

  // Initialize LoRa module
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Failed to initialize LoRa.");
    while (1);
  }
}

void loop() {
  // Measure current and calculate power
  float current = getCurrent();
  float currentInAmperes = current / 1000.0;
  float power = currentInAmperes * SUPPLY_VOLTAGE;

  // Calculate elapsed time for energy computation
  static unsigned long previousMillis = millis();
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  // Calculate energy consumed in this period
  float energyConsumed = (power * (elapsedTime / 1000.0)) / 3600.0; // Convert to Wh
  cumulativeEnergy += energyConsumed;

  // Send cumulative energy via LoRa
  String dataToSend = String(cumulativeEnergy, 2);
  LoRa.beginPacket();
  LoRa.print(dataToSend);
  LoRa.endPacket();

  // Debugging output
  Serial.print("Current: ");
  Serial.print(current, 2);
  Serial.print(" mA, Power: ");
  Serial.print(power, 2);
  Serial.print(" W, Cumulative Energy: ");
  Serial.println(cumulativeEnergy, 4);

  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cumulative Energy:");
  lcd.setCursor(0, 1);
  lcd.print(cumulativeEnergy, 2);
  lcd.print(" Wh");

  // Update EEPROM if cumulative energy has changed significantly
  if (abs(cumulativeEnergy - previousCumulativeEnergy) >= ENERGY_THRESHOLD) {
    EEPROM_writeFloat(EEPROM_ADDRESS, cumulativeEnergy);
    Serial.println("Cumulative Energy saved to EEPROM.");
    previousCumulativeEnergy = cumulativeEnergy;
  }

  previousMillis = currentMillis;

  delay(1000);
}