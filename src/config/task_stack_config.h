#pragma once

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "utils/task_stack/task_stack_monitor.h"

namespace TaskStackConfig
{
    extern const TickType_t kMonitorPeriodTicks;
    extern const uint32_t kWarnFreeBytes;
    extern const TaskStackMonitor::TaskConfig kTasks[];
    extern const size_t kTaskCount;
}

