#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

void Config_Init(void);

uint8_t Config_GetRetryAttempts(void);
uint32_t Config_GetAckTimeoutMs(void);
uint16_t Config_GetBenchmarkPackets(void);
uint16_t Config_GetChaosPackets(void);

bool Config_SetRetryAttempts(
    uint8_t attempts
);

bool Config_SetAckTimeoutMs(
    uint32_t timeout_ms
);

bool Config_SetBenchmarkPackets(
    uint16_t packets
);

bool Config_SetChaosPackets(
    uint16_t packets
);


uint8_t Config_GetNormalRate(void);
uint8_t Config_GetCrcRate(void);
uint8_t Config_GetDropRate(void);
uint8_t Config_GetDuplicateRate(void);
uint8_t Config_GetDelayRate(void);

bool Config_SetFaultRates(
    uint8_t normal,
    uint8_t crc,
    uint8_t drop,
    uint8_t duplicate,
    uint8_t delay
);

void Config_Print(void);

#endif