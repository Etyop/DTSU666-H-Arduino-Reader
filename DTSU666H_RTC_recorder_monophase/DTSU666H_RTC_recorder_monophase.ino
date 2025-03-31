#include <SPI.h>  // Include all the libraries needed for this program
#include <SD.h>
#include <ModbusMaster.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "RTClib.h"


#define MAX485_DE 3      // DE MAX485 pin on the Arduino
#define MAX485_RE_NEG 2  // RE MAX485 pin on the Arduino, can be the same as DE
#define BUTTON_PIN 4
#define LONG_PRESS 5000  // Length holding down the button required to change page on the LCD screen (in ms)
#define CS_SD_PIN 53     // CS SD adapter pin on the Arduino

ModbusMaster node;  // Create a Modbus node named node

RTC_DS1307 RTC;

String date;

LiquidCrystal_I2C lcd(0x27, 20, 4);  // Create a lcd object at the address 0x27, 16 pixels wide, 2 pixels high

int currentPage = 1;  // Default page on the LCD
const int totalPages = 3;

float Ua, Ia, Pa, Qa, Sa, F;  // Write here the name of all the values you want to get from Modbus (read the readme to see full list)

File dataFile;  // Create a file object named dataFile

bool backlightOff = false;  // Required to turn the LCD screen on or off

unsigned long previousMillis = 0;  // Set the timer to 0
const long interval = 1000;        // Interval at which everything refreshes (in ms)

bool buttonState = HIGH;  // Button debouncing
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long buttonPressTime = 0;


void preTransmission() {  // Required for Modbus communication, do not modify even if you'll only recept data
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}


void postTransmission() {  // Required for Modbus communication, do not modify even if you'll only recept data
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}


void updateVariables() {      // This function stores the Modbus Data being read at the register address in the variable name
  Ua = readModbusData(2110);  // To add new variables to be stored, they need to be first created alongside the existing ones at the beginning of the program.
  Ia = readModbusData(2102);
  Pa = readModbusData(2128);
  Qa = readModbusData(2136);
  Sa = readModbusData(2144);
  F = readModbusData(2124);
}


float readModbusData(int registerAddress) {                        // This function is used to read Modbus data (no way ???)
  uint8_t result = node.readHoldingRegisters(registerAddress, 2);  // We store the holding register at the specified register address, which is made of 2 registers (one lower register, one upper register)

  if (result == node.ku8MBSuccess) {             // If we have a value stored, it means we successfully retrieved the register
    uint16_t data1 = node.getResponseBuffer(0);  // Store the lower register and the upper register in seperate variables
    uint16_t data2 = node.getResponseBuffer(1);
    uint32_t combinedValue = (static_cast<uint32_t>(data1) << 16) | data2;  // Combine the lower and the upper to get the real register value
    float value;
    memcpy(&value, &combinedValue, sizeof(float));  // Copy the memory block from combinedValue to value
    return value;
  } else {  // If we don't have any value stored, it means we failed to retrieve the register
    Serial.print("Failed to read register at address ");
    Serial.println(registerAddress);
    return 0.0;
  }
}


void openNewFile() {  // This function creates a new file on the SD card
  if (dataFile) {     // If by any mean a file is already opened, close it first
    dataFile.close();
  }

  char fileName[20];
  sprintf(fileName, "%lu.csv", millis());    // The file name consists of the actual millisecond at which this function is executed, followed by the csv format
  dataFile = SD.open(fileName, FILE_WRITE);  // Open the file in write mode. It will automatically create the file if it doesn't exist

  if (!dataFile) {  // Do I really need to comment this ?
    Serial.print("Error opening file! ");
    Serial.println(fileName);
  } else {
    Serial.print("File opened successfully: ");
    Serial.println(fileName);
  }
}


void printDataToSerial() {  // This function prints the data we retrieved into the serial monitor

  Serial.println(date);
  Serial.print("Ua: ");  // I think it is self-explanatory and if you don't understand what's happening here,
  Serial.print(Ua, 2);   // this program is probably too advanced for you. Train yourself on easier programs.
  Serial.print(" V, Ia: ");
  Serial.print(Ia, 2);
  Serial.print(" A, Pa: ");
  Serial.println(Pa, 2);

  Serial.print("Freq: ");
  Serial.print(F, 1);
  Serial.println(" Hz");

  Serial.println("--------------------");
}


void initCsvOnSD() {                                                                 // This function creates the header line on the CSV file
  dataFile.println("Time;Ua;Ia;Pa;Qa;Sa;F");  // Remember to add your variables and check if you need ; or , (READ THE REAAAAADMEEEEE)
}


void printDataToSD() {                  // This function prints the data in the csv file
  unsigned long now = millis() / 1000;  // Divide millis by 1000 to get which second is the program currently at

  int sec = now % 60;  // Convert seconds into days hours mins secs
  int min = (now / 60) % 60;
  int hours = (now / 3600) % 24;
  int days = now / 3600 / 24;

  String strDays = String(days);  // Convert int in strings
  String strHours = String(hours);
  String strMin = String(min);
  String strSec = String(sec);

  String timeElapsed = "D" + strDays + " - " + strHours + ":" + strMin + ":" + strSec;  // Combine the strings and apply formatting




  dataFile.print(date);  // Prints every value on the same line
  dataFile.print(";");
  dataFile.print(Ua, 1);
  dataFile.print(";");
  dataFile.print(Ia, 2);
  dataFile.print(";");
  dataFile.print(Pa, 2);
  dataFile.print(";");
  dataFile.print(Qa, 2);
  dataFile.print(";");
  dataFile.print(Sa, 2);
  dataFile.print(";");
  dataFile.println(F, 1);  // Jump line at the end of this line to prepare for the next second

  dataFile.flush();  // Flush the memory on the SD card (in other words, write the data)
}


void printDataToLCD() {  // This function prints data to the LCD screen. It is dependent on the page number.
  lcd.clear();           // I think it is self-explanatory and if you don't understand what's happening here,
  lcd.setCursor(0, 0);   // this program is probably too advanced for you. Train yourself on easier programs.
  lcd.print("a: ");
  lcd.print(Ua, 1);
  lcd.print("V ");
  lcd.print(Ia, 2);
  lcd.print("A");
  lcd.setCursor(0, 3);
  lcd.print("P: ");
  lcd.print(Pa, 0);
}


void turnOffBacklight() {  // No need to explain
  lcd.noBacklight();
  backlightOff = true;
}


void turnOnBacklight() {  // same
  lcd.backlight();
  backlightOff = false;
}


void setup() {  // As in every Arduino program, this function is mandatory.
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);

  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial2.begin(9600, SERIAL_8N1);  // Open Serial2 (TX2,RX2) at 9600 baud, 8 data bits, no parity, 1 stop bit. These settings need to be exactly the same as your Modbus device. Read your documentation.
  node.begin(11, Serial2);          // Begin communication on Serial2 with Modbus address 11. This needs to be exactly the same as your Modbus device. Read your documentation.

  Serial.begin(9600);  // Open Serial (serial monitor on USB) at 9600 baud
  Serial.println("Starting Modbus Transmission:");
  node.preTransmission(preTransmission);  // Initialize the Modbus Transmission
  node.postTransmission(postTransmission);

  Wire.begin();
  RTC.begin();

  // RTC.adjust(DateTime(__DATE__, __TIME__));

  lcd.init();       // Turn on the LCD screen
  lcd.backlight();  // Turn on the LCD backlight

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // set the button as an input_pullup (read Arduino docs if you don't know what it is)

  if (!SD.begin(CS_SD_PIN)) {
    Serial.println("Error initializing SD card!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur SD");
    delay(5000);
    return;
  } else {
    Serial.println("SD card initialized successfully.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD OK");
    delay(2500);
  }

  openNewFile();
  initCsvOnSD();
}


void loop() {                              // Main program loop
  unsigned long currentMillis = millis();  // Write at which millis() we are everytime to loop comes back to the beginning

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {  // This whole part is for the button. First we check if the button has changed state since last time the loop came here
    lastDebounceTime = currentMillis;
  }

  if ((currentMillis - lastDebounceTime) > debounceDelay) {  // Debouncing stuff
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {  // this part switches the backlight on or off if pressed for longer than LONG_PRESS, and switches pages if pressed for shorter than LONG_PRESS
        buttonPressTime = currentMillis;
      } else {
        if (backlightOff) {
          turnOnBacklight();
        } else {
          if (currentMillis - buttonPressTime > LONG_PRESS) {
            turnOffBacklight();
          }
        }
      }
    }
  }
  lastButtonState = reading;

  if (currentMillis - previousMillis >= interval) {  // every interval (in our case, 1000ms)
    DateTime now = RTC.now();

    String strDay = String(now.day());
    String strMonth = String(now.month());
    String strYear = String(now.year());
    String strHour = String(now.hour());
    String strMinute = String(now.minute());
    String strSecond = String(now.second());

    date = strDay + "/" + strMonth + "/" + strYear + " " + strHour + ":" + strMinute + ":" + strSecond;

    previousMillis = currentMillis;

    updateVariables();

    printDataToLCD();
    printDataToSerial();
    printDataToSD();
  }
}