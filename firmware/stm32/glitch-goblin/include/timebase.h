#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

void Timebase_Init(void);
uint32_t Timebase_Millis(void);
void Timebase_DelayMs(uint32_t delay_ms);

#endif