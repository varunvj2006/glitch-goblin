#include <Arduino.h>

#include "serum_link.h"
#include "fault_engine.h"
#include "console.h"
#include "config.h"

void setup()
{
    Serial.begin(115200);

    SerumLink_Init();
    FaultEngine_Init();

    Serial.setTimeout(2000);

    delay(1000);

    Console_PrintWelcome();
}

void loop()
{
    Console_Process();
}