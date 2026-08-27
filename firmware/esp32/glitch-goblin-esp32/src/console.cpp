#include <Arduino.h>

#include "console.h"
#include "test_runner.h"
#include "fault_engine.h"
#include "config.h"


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
        "bench <1-10000>"
    );

    Serial.println(
        "chaos"
    );

    Serial.println(
        "chaos <1-10000>"
    );

    Serial.println(
        "retries <1-10>"
    );

    Serial.println(
        "timeout <50-5000>"
    );

    Serial.println(
        "config"
    );
    Serial.println(
    "faults <normal> <crc> <drop> <duplicate> <delay>"
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
        command.startsWith(
            "bench "
        )
    )
    {
        uint32_t packets =
            command.substring(6).toInt();

        if (
            Config_SetBenchmarkPackets(
                packets
            )
        )
        {
            Serial.print(
                "Benchmark packets set to "
            );

            Serial.println(
                packets
            );

            TestRunner_RunBenchmark();
        }
        else
        {
            Serial.println(
                "Invalid benchmark size"
            );
        }
    }


    else if (
        command ==
        "bench"
    )
    {
        TestRunner_RunBenchmark();
    }


    else if (
        command.startsWith(
            "chaos "
        )
    )
    {
        uint32_t packets =
            command.substring(6).toInt();

        if (
            Config_SetChaosPackets(
                packets
            )
        )
        {
            Serial.print(
                "Chaos packets set to "
            );

            Serial.println(
                packets
            );

            TestRunner_RunChaos();
        }
        else
        {
            Serial.println(
                "Invalid chaos size"
            );
        }
    }


    else if (
        command ==
        "chaos"
    )
    {
        TestRunner_RunChaos();
    }


    else if (
        command.startsWith(
            "retries "
        )
    )
    {
        uint32_t attempts =
            command.substring(8).toInt();

        if (
            Config_SetRetryAttempts(
                attempts
            )
        )
        {
            Serial.print(
                "Retry attempts set to "
            );

            Serial.println(
                attempts
            );
        }
        else
        {
            Serial.println(
                "Retries must be 1-10"
            );
        }
    }


    else if (
        command.startsWith(
            "timeout "
        )
    )
    {
        uint32_t timeout_ms =
            command.substring(8).toInt();

        if (
            Config_SetAckTimeoutMs(
                timeout_ms
            )
        )
        {
            Serial.print(
                "ACK timeout set to "
            );

            Serial.print(
                timeout_ms
            );

            Serial.println(
                " ms"
            );
        }
        else
        {
            Serial.println(
                "Timeout must be 50-5000 ms"
            );
        }
    }


    else if (
        command ==
        "config"
    )
    {
        Config_Print();
    }


    else if (
        command ==
        "help"
    )
    {
        PrintHelp();
    }
    else if (
        command.startsWith(
            "faults "
        )
    )
    {
        int normal;
        int crc;
        int drop;
        int duplicate;
        int delay;

        int parsed =
            sscanf(
                command.c_str(),
                "faults %d %d %d %d %d",
                &normal,
                &crc,
                &drop,
                &duplicate,
                &delay
            );

        if (parsed != 5)
        {
            Serial.println(
                "Usage: faults <normal> <crc> <drop> <duplicate> <delay>"
            );
        }

        else if (
            normal < 0 ||
            normal > 100 ||
            crc < 0 ||
            crc > 100 ||
            drop < 0 ||
            drop > 100 ||
            duplicate < 0 ||
            duplicate > 100 ||
            delay < 0 ||
            delay > 100
        )
        {
            Serial.println(
                "Each fault rate must be 0-100"
            );
        }

        else if (
            !Config_SetFaultRates(
                normal,
                crc,
                drop,
                duplicate,
                delay
            )
        )
        {
            Serial.println(
                "Fault percentages must add up to 100"
            );
        }

        else
        {
            Serial.println(
                "Fault probabilities updated"
            );

            Config_Print();
        }
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