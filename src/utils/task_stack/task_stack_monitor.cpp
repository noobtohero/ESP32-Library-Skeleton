#include "task_stack_monitor.h"

#include "Logger.h"
#include "freertos/task.h"

namespace TaskStackMonitor
{
    void logTaskStackByName(const char *taskName, uint32_t stackBytes, uint32_t warnFreeBytes)
    {
        TaskHandle_t handle = xTaskGetHandle(taskName);
        if (handle == NULL)
        {
            log("STACK %s: not found", taskName);
            return;
        }

        uint32_t minFreeBytes = static_cast<uint32_t>(uxTaskGetStackHighWaterMark(handle));
        uint32_t usedMaxBytes = (stackBytes > minFreeBytes) ? (stackBytes - minFreeBytes) : 0U;
        uint32_t usedPercent = (stackBytes > 0U) ? ((usedMaxBytes * 100U) / stackBytes) : 0U;
        const char *level = (minFreeBytes < warnFreeBytes) ? "WARN" : "OK";

        log("STACK %s: freeMin=%luB usedMax=%lu/%luB (%lu%%) %s",
            taskName,
            static_cast<unsigned long>(minFreeBytes),
            static_cast<unsigned long>(usedMaxBytes),
            static_cast<unsigned long>(stackBytes),
            static_cast<unsigned long>(usedPercent),
            level);
    }

    void logStackUsageOnce(TickType_t now,
                           TickType_t &lastTick,
                           TickType_t periodTicks,
                           const TaskConfig *tasks,
                           size_t taskCount,
                           uint32_t warnFreeBytes)
    {
        if ((now - lastTick) < periodTicks)
        {
            return;
        }

        lastTick = now;

        for (size_t i = 0; i < taskCount; i++)
        {
            logTaskStackByName(tasks[i].name, tasks[i].stackBytes, warnFreeBytes);
        }
    }
}

