#include <Arduino.h>
#include "serum.h"

HardwareSerial SerumUART(2);

SerumParser serum_parser;
typedef enum
{
    FAULT_NONE,
    FAULT_BAD_CRC,
    FAULT_DROP,
    FAULT_DUPLICATE,
    FAULT_DELAY

} FaultMode;



bool WaitForAck(
    uint16_t sequence,
    uint32_t timeout_ms
)
{
    uint32_t start = millis();

    while (millis() - start < timeout_ms)
    {
        while (SerumUART.available())
        {
            uint8_t byte =
                SerumUART.read();

            SerumParseResult result =
                SerumParser_ProcessByte(
                    &serum_parser,
                    byte
                );

            if (
                result ==
                SERUM_PARSE_PACKET_READY
            )
            {
                if (
                    serum_parser.packet.type ==
                        SERUM_MSG_ACK &&
                    serum_parser.packet.sequence ==
                        sequence
                )
                {
                    return true;
                }
            }
        }

        delay(1);
    }

    return false;
}
void SendSerumPing(
    uint16_t sequence,
    FaultMode fault
)
{
    SerumPacket ping = {0};

    ping.version = SERUM_VERSION;
    ping.type = SERUM_MSG_PING;
    ping.length = 0;
    ping.sequence = sequence;

    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            &ping,
            tx_buffer,
            sizeof(tx_buffer)
        );

    if (tx_length == 0)
    {
        Serial.println("Packet encoding failed");
        return;
    }

    if (fault == FAULT_BAD_CRC)
    {
        tx_buffer[tx_length - 1] ^= 0x01;
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" sent [BAD CRC]");

        SerumUART.write(
            tx_buffer,
            tx_length
        );

        return;
    }

    if (fault == FAULT_DROP)
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" [DROPPED]");

        return;
    }

    if (fault == FAULT_DELAY)
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" [DELAYED]");

        delay(2000);
    }

    SerumUART.write(
        tx_buffer,
        tx_length
    );

    Serial.print("PING #");
    Serial.print(sequence);

    if (fault == FAULT_DUPLICATE)
    {
        Serial.println(" sent [DUPLICATE]");

        delay(50);

        SerumUART.write(
            tx_buffer,
            tx_length
        );
    }
    else
    {
        Serial.println(" sent [VALID]");
    }
}

void setup()
{
    Serial.begin(115200);

    SerumUART.begin(
        115200,
        SERIAL_8N1,
        16,
        17
    );

    SerumParser_Init(
        &serum_parser
    );

    delay(1000);

    Serial.println("ESP32 ready");

    int passed = 0;
    int total = 5;

    Serial.println();
    Serial.println("=== GLITCH GOBLIN SERUM TEST ===");
    Serial.println();


    Serial.println("[1] NORMAL");

    SendSerumPing(
        1,
        FAULT_NONE
    );

    if (WaitForAck(1, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[2] BAD CRC");

    SendSerumPing(
        2,
        FAULT_BAD_CRC
    );

    if (!WaitForAck(2, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[3] DROP");

    SendSerumPing(
        3,
        FAULT_DROP
    );

    if (!WaitForAck(3, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[4] DUPLICATE");

    SendSerumPing(
        4,
        FAULT_DUPLICATE
    );

    if (WaitForAck(4, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[5] DELAY");

    SendSerumPing(
        5,
        FAULT_DELAY
    );

    if (WaitForAck(5, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }


    Serial.println();
    Serial.println("==============================");

    Serial.print("RESULTS: ");
    Serial.print(passed);
    Serial.print(" / ");
    Serial.print(total);
    Serial.println(" PASSED");

    Serial.println("==============================");

}

void loop()
{
    
}