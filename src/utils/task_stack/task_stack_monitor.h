#pragma once

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

namespace TaskStackMonitor
{
    struct TaskConfig
    {
        const char *name;
        uint32_t stackBytes;
    };

    void logTaskStackByName(const char *taskName, uint32_t stackBytes, uint32_t warnFreeBytes);

    void logStackUsageOnce(TickType_t now,
                           TickType_t &lastTick,
                           TickType_t periodTicks,
                           const TaskConfig *tasks,
                           size_t taskCount,
                           uint32_t warnFreeBytes);
}

