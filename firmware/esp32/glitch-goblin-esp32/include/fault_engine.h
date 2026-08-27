#ifndef FAULT_ENGINE_H
#define FAULT_ENGINE_H

#include <stdint.h>

typedef enum
{
    FAULT_NONE,
    FAULT_BAD_CRC,
    FAULT_DROP,
    FAULT_DUPLICATE,
    FAULT_DELAY
} FaultMode;

void FaultEngine_Init(void);

FaultMode FaultEngine_GetRandomFault(void);

const char *FaultEngine_GetName(
    FaultMode fault
);

void FaultEngine_SendWithFault(
    const uint8_t *buffer,
    uint16_t length,
    FaultMode fault
);

#endif