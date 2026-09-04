/*
  EcoNode - DIAGNOSTIC VERSION (no BLE)
  Use this to test whether the BMP180 sensor and library work correctly
  on their own, before adding BLE back in. If this runs fine and prints
  readings, the problem is in the BLE section, not the sensor.

  Wokwi setup: same as before - ESP32-C3, BMP180 part, Adafruit BMP085
  Library added via Library Manager.
*/

#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== EcoNode diagnostic start ===");

  Wire.begin(8, 9);

  if (!bmp.begin()) {
    Serial.println("BMP180 not found - check wiring / library");
  } else {
    Serial.println("BMP180 found successfully!");
    float tempC = bmp.readTemperature();
    Serial.printf("Temp: %.2f C\n", tempC);
  }
}

void loop() {
  Serial.println("Loop running - sketch loaded fine.");
  delay(2000);
}
