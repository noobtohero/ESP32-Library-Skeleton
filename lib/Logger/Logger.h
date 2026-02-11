#ifndef FREERTOS_LOGGER_H
#define FREERTOS_LOGGER_H

#include <Arduino.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef DEBUG_MODE
/**
 * @brief Log ข้อความและขึ้นบรรทัดให้ม่ (\n)
 */
void log(const char *fmt, ...);

/**
 * @brief Log ข้อความโดยไม่ขึ้นบรรทัดให้ม่
 */
void log_raw(const char *fmt, ...);
#else
static inline void log(const char *fmt, ...) { (void)fmt; }
static inline void log_raw(const char *fmt, ...) { (void)fmt; }
#endif

#endif // FREERTOS_LOGGER_H
