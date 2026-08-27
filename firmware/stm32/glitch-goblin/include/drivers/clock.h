#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void Clock_Init84MHz(void);

uint32_t Clock_GetAPB1Hz(void);
uint32_t Clock_GetAPB2Hz(void);

#endif