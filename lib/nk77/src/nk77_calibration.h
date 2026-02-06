#pragma once
#include <stdint.h>

bool nk77_processPulse(uint32_t pulse);
void nk77_factoryReset();

int getLastPulseCount();
