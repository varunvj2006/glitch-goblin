#include <Arduino.h>

#include "console.h"
#include "test_runner.h"
#include "fault_engine.h"

static void PrintHelp(void)
{
    Serial.println();

    Serial.println(
        "Commands:"
    );

    Serial.println(
        "normal"
    );

    Serial.println(
        "crc"
    );

    Serial.println(
        "drop"
    );

    Serial.println(
        "duplicate"
    );

    Serial.println(
        "delay"
    );

    Serial.println(
        "stats"
    );

    Serial.println(
        "reliable"
    );

    Serial.println(
        "bench"
    );

    Serial.println(
        "chaos"
    );

    Serial.println(
        "help"
    );
}

void Console_PrintWelcome(void)
{
    Serial.println();

    Serial.println(
        "=== GLITCH GOBLIN ==="
    );

    PrintHelp();

    Serial.println();
}

void Console_Process(void)
{
    if (
        !Serial.available()
    )
    {
        return;
    }

    String command =
        Serial.readStringUntil(
            '\n'
        );

    command.trim();

    command.toLowerCase();

    if (
        command ==
        "normal"
    )
    {
        TestRunner_RunFault(
            FAULT_NONE,
            true
        );
    }

    else if (
        command ==
        "crc"
    )
    {
        TestRunner_RunFault(
            FAULT_BAD_CRC,
            false
        );
    }

    else if (
        command ==
        "drop"
    )
    {
        TestRunner_RunFault(
            FAULT_DROP,
            false
        );
    }

    else if (
        command ==
        "duplicate"
    )
    {
        TestRunner_RunFault(
            FAULT_DUPLICATE,
            true
        );
    }

    else if (
        command ==
        "delay"
    )
    {
        TestRunner_RunFault(
            FAULT_DELAY,
            true
        );
    }

    else if (
        command ==
        "stats"
    )
    {
        TestRunner_RunStats();
    }

    else if (
        command ==
        "reliable"
    )
    {
        TestRunner_RunReliablePing();
    }

    else if (
        command ==
        "bench"
    )
    {
        TestRunner_RunBenchmark();
    }

    else if (
        command ==
        "chaos"
    )
    {
        TestRunner_RunChaos();
    }

    else if (
        command ==
        "help"
    )
    {
        PrintHelp();
    }

    else
    {
        Serial.print(
            "Unknown command: "
        );

        Serial.println(
            command
        );
    }

    Serial.println();
}