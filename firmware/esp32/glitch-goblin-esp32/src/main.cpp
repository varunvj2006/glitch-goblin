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

    uint8_t ping_packet[] =
    {
        0x53,
        0x45,
        0x01,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0xF1,
        0xC0
    };

    SerumUART.write(
        ping_packet,
        sizeof(ping_packet)
    );

    Serial.println("SERUM PING #1 sent");
}

void loop()
{
    while (SerumUART.available())
    {
        uint8_t byte =
            SerumUART.read();

        if (byte < 0x10)
        {
            Serial.print("0");
        }

        Serial.print(byte, HEX);
        Serial.print(" ");
    }
}