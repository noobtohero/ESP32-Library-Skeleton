#include "config/task_stack_config.h"

namespace TaskStackConfig
{
    const TickType_t kMonitorPeriodTicks = pdMS_TO_TICKS(10000);
    const uint32_t kWarnFreeBytes = 512;

    const TaskStackMonitor::TaskConfig kTasks[] = {
        {"mainTask", 4096},
        {"nk77Pulse", 8000},
        {"lcdTask", 3072},
        {"LogTask", 4096},
        {"macroAddTimes", 4096},
    };

    const size_t kTaskCount = sizeof(kTasks) / sizeof(kTasks[0]);
}

