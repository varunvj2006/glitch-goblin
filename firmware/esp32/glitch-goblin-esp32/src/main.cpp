#include <Arduino.h>

HardwareSerial SerumUART(2);

void setup()
{
    Serial.begin(115200);

    SerumUART.begin(
        115200,
        SERIAL_8N1,
        16,
        17
    );

    delay(1000);

    Serial.println("ESP32 ready");

    SerumUART.println("HELLO STM32");
}

void loop()
{
    while (SerumUART.available())
    {
        char byte = SerumUART.read();
        Serial.write(byte);
    }
}