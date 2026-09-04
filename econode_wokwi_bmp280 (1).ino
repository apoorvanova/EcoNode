/*
  EcoNode - Wokwi simulation variant
  Uses BMP280 instead of BME280, because Wokwi's built-in parts library
  does not include a native BME280 (only BMP280). Same chip family, same
  I2C wiring - this variant just doesn't report humidity.

  Your real KiCad schematic and physical design still use the actual
  BME280 (a real, purchasable chip with humidity sensing) - this is
  purely a simulation-environment substitution, worth mentioning honestly
  in your LinkedIn post as a "simulated with the closest available part."

  Wokwi setup:
    1. New project -> "ESP32-C3"
    2. Add a "BMP280" part from the parts panel, wire:
         VCC -> 3V3, GND -> GND, SCL -> GPIO9, SDA -> GPIO8
    3. Paste this sketch, hit play.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SEA_LEVEL_HPA   (1013.25)
#define SLEEP_SECONDS   600
#define SERVICE_UUID    "0000181A-0000-1000-8000-00805F9B34FB"
#define CHAR_UUID       "00002A6E-0000-1000-8000-00805F9B34FB"

Adafruit_BMP280 bmp;
RTC_DATA_ATTR int bootCount = 0;

void advertiseReading(float tempC) {
  BLEDevice::init("EcoNode-01");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pChar = pService->createCharacteristic(
      CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pChar->addDescriptor(new BLE2902());

  int16_t tempX100 = (int16_t)(tempC * 100);
  pChar->setValue((uint8_t *)&tempX100, 2);

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE advertising started for 3s...");
  delay(3000);
  pAdvertising->stop();
  BLEDevice::deinit(true);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  bootCount++;

  Serial.println("=================================");
  Serial.printf("EcoNode wake #%d\n", bootCount);

  Wire.begin(8, 9);   // SDA = GPIO8, SCL = GPIO9 on ESP32-C3

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found - check wiring");
  } else {
    float tempC    = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F;
    float altitude = bmp.readAltitude(SEA_LEVEL_HPA);

    Serial.printf("Temp:     %.2f C\n", tempC);
    Serial.printf("Pressure: %.2f hPa\n", pressure);
    Serial.printf("Altitude: %.2f m\n", altitude);
    Serial.println("(Humidity not available - BMP280 lacks a humidity sensor;");
    Serial.println(" the real board uses BME280, which does have one.)");

    advertiseReading(tempC);
  }

  Serial.printf("Sleeping for %d seconds...\n", SLEEP_SECONDS);
  Serial.println("=================================");
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // never reached - deep sleep restarts execution at setup()
}
