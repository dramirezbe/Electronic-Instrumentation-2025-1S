#include <SPI.h>              // Required for SPI communication
#include <Adafruit_BMP280.h>  // Library for BMP280 sensor

// --- Define SPI Pins for BMP280 ---
// ESP32 default VSPI pins:
#define ESP32_SCLK_PIN  18  // Connects to BMP280's SCL pin
#define ESP32_MISO_PIN  19  // Connects to BMP280's SDD pin (or SDO)
#define ESP32_MOSI_PIN  23  // Connects to BMP280's SDA pin (or SDI/DIN)
#define BMP_CS_PIN      5   // Connects to BMP280's CSB pin (Chip Select)

// Create an Adafruit_BMP280 object for Hardware SPI communication
// The constructor needs the CS, MOSI, MISO, and SCK pins in that order.
// Map BMP280 pins to ESP32 SPI roles:
// BMP280_CSB (active low) -> ESP32_CS_PIN
// BMP280_SDA (MOSI/SDI)   -> ESP32_MOSI_PIN
// BMP280_SDD (MISO/SDO)   -> ESP32_MISO_PIN
// BMP280_SCL (SCLK)      -> ESP32_SCLK_PIN
Adafruit_BMP280 bmp(BMP_CS_PIN, ESP32_MOSI_PIN, ESP32_MISO_PIN, ESP32_SCLK_PIN);

void setup() {
  Serial.begin(115200); // Initialize serial communication for debugging

  Serial.println(F("BMP280 test - ESP32 WROVER SPI (SCL, SDA, CSB, SDD module)"));
  Serial.println(F("SPI Wiring:"));
  Serial.print(F("  BMP280 SCL (CLK)  -> ESP32 GPIO ")); Serial.println(ESP32_SCLK_PIN);
  Serial.print(F("  BMP280 SDA (MOSI) -> ESP32 GPIO ")); Serial.println(ESP32_MOSI_PIN);
  Serial.print(F("  BMP280 SDD (MISO) -> ESP32 GPIO ")); Serial.println(ESP32_MISO_PIN);
  Serial.print(F("  BMP280 CSB (CS)   -> ESP32 GPIO ")); Serial.println(BMP_CS_PIN);
  Serial.println(F(""));
  Serial.println(F("!!! IMPORTANT: Connect BMP280 CSB pin to ESP32 GND to enable SPI mode !!!"));
  Serial.println(F(""));

  unsigned status;
  // Initialize the BMP280 sensor via SPI.
  status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring and ensure:"));
    Serial.println(F("  1. BMP280 CSB pin is connected to ESP32 GND."));
    Serial.println(F("  2. Wiring matches the defined SPI pins."));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
    Serial.println(F("    ID of 0xFF probably means a bad connection."));
    Serial.println(F("    ID of 0x56-0x58 represents a BMP 280."));
    Serial.println(F("    ID of 0x60 represents a BME 280."));
    Serial.println(F("    ID of 0x61 represents a BME 680."));
    while (1) delay(10); // Halt execution if sensor not found
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,      /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,      /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,     /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,       /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);  /* Standby time. */
}

void loop() {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1011.9)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();
    delay(2000);
}