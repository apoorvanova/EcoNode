/*
  EcoNode - Solar-powered BLE environmental sensor node
  Simulated in Wokwi: ESP32 + BME280

  Wake -> read sensor -> advertise over BLE -> deep sleep -> repeat.
  This is the core power-saving pattern used in real battery/solar IoT nodes:
  the MCU spends >99% of its life in deep sleep (~20uA) and only wakes
  briefly (~200ms, ~120mA) to sense and transmit.

  Wokwi setup:
    1. New project -> "ESP32-C3" (not plain "ESP32" - different chip, different pins)
    2. Add a "BME280" part from the parts panel, wire:
         VCC -> 3V3, GND -> GND, SCL -> GPIO9, SDA -> GPIO8
    3. Paste this sketch, hit play.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SEA_LEVEL_HPA   (1013.25)
#define SLEEP_SECONDS   600        // 10 minute duty cycle
#define SERVICE_UUID    "0000181A-0000-1000-8000-00805F9B34FB" // BLE Environmental Sensing
#define CHAR_UUID       "00002A6E-0000-1000-8000-00805F9B34FB" // Temperature characteristic

Adafruit_BME280 bme;
RTC_DATA_ATTR int bootCount = 0;   // survives deep sleep, useful for battery-life logging

void advertiseReading(float tempC, float humidity, float pressureHpa) {
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
  delay(3000);   // real node would advertise briefly, then stop to save power
  pAdvertising->stop();
  BLEDevice::deinit(true);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  bootCount++;

  Serial.println("=================================");
  Serial.printf("EcoNode wake #%d\n", bootCount);

  Wire.begin(8, 9);   // SDA = GPIO8, SCL = GPIO9 on ESP32-C3 (GPIO21/22 is the classic-ESP32 default, not valid here)
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found - check wiring");
  } else {
    float tempC     = bme.readTemperature();
    float pressure  = bme.readPressure() / 100.0F;
    float humidity  = bme.readHumidity();
    float altitude  = bme.readAltitude(SEA_LEVEL_HPA);

    Serial.printf("Temp:     %.2f C\n", tempC);
    Serial.printf("Humidity: %.2f %%\n", humidity);
    Serial.printf("Pressure: %.2f hPa\n", pressure);
    Serial.printf("Altitude: %.2f m\n", altitude);

    advertiseReading(tempC, humidity, pressure);
  }

  Serial.printf("Sleeping for %d seconds...\n", SLEEP_SECONDS);
  Serial.println("=================================");
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // never reached - deep sleep restarts execution at setup()
}
