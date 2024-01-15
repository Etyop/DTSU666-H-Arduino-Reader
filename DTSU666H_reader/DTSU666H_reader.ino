#include <ModbusMaster.h>

#define MAX485_DE      3     // DE MAX485 pin on the Arduino
#define MAX485_RE_NEG  2     // RE MAX485 pin on the Arduino, can be the same as DE

ModbusMaster node;            // Creates a Modbus node named node.

void preTransmission() {              // Mandatory to setup the MAX485 properly, even if used only for reception
    digitalWrite(MAX485_RE_NEG, 1);
    digitalWrite(MAX485_DE, 1);
}

void postTransmission() {             // Same
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
}

void setup() {
    pinMode(MAX485_RE_NEG, OUTPUT);   // Sets the pins as outputs (they will set RE and DE on or off to tell the MAX485 if its reading or writing)
    pinMode(MAX485_DE, OUTPUT);


    digitalWrite(MAX485_RE_NEG, 0);    // Init in receive mode by defining both to 0. If you need to transmit you need to define both to 1, thats why they can be both on the same pin.
    digitalWrite(MAX485_DE, 0);  

    Serial2.begin(9600, SERIAL_8N1);  // Starts the serial communication on Serial2 (TX2, RX2), 9600 baud, 8 bits - No parity - 1 stop bit

    node.begin(11, Serial2);          // We begin the node on address 11 (can be different on your side, read your documentation) through Serial2

    Serial.begin(9600);               // Starts the serial monitor on USB (Serial0)
    Serial.println("Starting Modbus Transaction:");
    node.preTransmission(preTransmission);    // Init stuff
    node.postTransmission(postTransmission);
}

void printData(const char* name, int registerAddress, const char* unit) {   // Creates a function with 3 parameters
    uint8_t result = node.readHoldingRegisters(registerAddress, 2);         // Stores the 2 bytes of the register at the address we chose

    if (result == node.ku8MBSuccess) {                    // If there is something in result, it works
        uint16_t data1 = node.getResponseBuffer(0);       // Stores the lower register in data1
        uint16_t data2 = node.getResponseBuffer(1);       // Stores the upper register in data2

        uint32_t combinedValue = (static_cast<uint32_t>(data1) << 16) | data2;  // Combines both registers to get full register

        float value;                                      // Interprets the combined value as a float
        memcpy(&value, &combinedValue, sizeof(float));

        Serial.print(name);
        Serial.print(" : ");
        Serial.print(value, 2); // prints with 2 decimals, set as you need
        Serial.print(" ");
        Serial.println(unit);
    } else {                    // If there is nothing in result, it doesn't work
        Serial.print("Failed to read ");
        Serial.println(name);
    }
}

void loop() {
    printData("Ua", 2110, "V");                 // Uses the printData function just above, with the name, then the register address, then the unit
    printData("Ub", 2112, "V");
    printData("Uc", 2114, "V");
    printData("Ia", 2102, "A");
    printData("Ib", 2104, "A");
    printData("Ic", 2106, "A");
    printData("Pa", 2128, "W");
    printData("Pb", 2130, "W");
    printData("Pc", 2132, "W");
    Serial.println("---------------------");  // Separation

    delay(1000); // Waits 1 second before reading again
}
