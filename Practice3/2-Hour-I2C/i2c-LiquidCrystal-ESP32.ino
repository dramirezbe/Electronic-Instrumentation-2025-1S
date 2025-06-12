#include <Wire.h>               // Required for I2C communication (even if not explicitly used for custom pins in this example, it's good practice for I2C)
#include <LiquidCrystal_I2C.h>  // Library for I2C LCD

// --- I2C LCD Configuration (from your example) ---
// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
// Using 0x27 as per your example, adjust if your LCD has a different address
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

// --- Potentiometer Configuration ---
const int POT_PIN = 34; // Analog pin connected to the potentiometer (e.g., GPIO 34, 35, 36)

// --- Clock Variables ---
unsigned long previousMillis = 0; // Stores last time clock was updated
const long interval = 1000;       // Interval at which to update the clock (1 second)
int hours = 0;
int minutes = 0;
int seconds = 0;

void setup() {
  Serial.begin(115200); // Initialize serial communication for debugging

  // Initialize the I2C bus. 
  // If you are using default ESP32 I2C pins (SDA=21, SCL=22), Wire.begin() is sufficient.
  // If you want to use custom pins like 27 and 26 as discussed before, uncomment and use:
  // Wire.begin(27, 26); 

  // initialize LCD (from your example)
  lcd.init();
  // turn on LCD backlight (from your example)
  lcd.backlight(); 

  // Initial display message
  lcd.setCursor(0, 0);
  lcd.print("Digital Clock");
  lcd.setCursor(0, 1);
  lcd.print("Potentiometer");
  delay(2000); // Show initial message for a bit
  lcd.clear(); // Clear the display before starting continuous updates
}

void loop() {
  // --- Digital Clock Logic ---
  unsigned long currentMillis = millis();

  // Update the clock every 'interval' milliseconds (1 second)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last time we updated

    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0; // Reset hours after 23 (midnight)
        }
      }
    }

    // Display clock on the first row (0)
    lcd.setCursor(0, 0); // Set cursor to column 0, row 0
    if (hours < 10) lcd.print("0"); // Add leading zero if hours is single digit
    lcd.print(hours);
    lcd.print(":");
    if (minutes < 10) lcd.print("0"); // Add leading zero if minutes is single digit
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0"); // Add leading zero if seconds is single digit
    lcd.print(seconds);
    lcd.print("  "); // Print spaces to clear any leftover characters from previous longer prints
  }

  // --- Potentiometer Logic ---
  int potValue = analogRead(POT_PIN); // Read the analog value (0-4095 for ESP32's 12-bit ADC)

  // Display potentiometer value on the second row (1)
  lcd.setCursor(0, 1); // Set cursor to column 0, row 1
  lcd.print("Pot: ");
  // Print the potentiometer value. Using spaces after to clear previous longer numbers.
  // If you want to map it to 0-100 or 0-5V, you'd use map() or float calculations here.
  lcd.print(potValue);
  lcd.print("      "); // Print spaces to clear any leftover characters

  // A small delay is often good practice in the loop to prevent very rapid updates
  // and give the microcontroller a breather, though with two lines updated independently,
  // it might not be strictly necessary for stability if your loop is fast.
  delay(10);
  lcd.backlight(); 
}