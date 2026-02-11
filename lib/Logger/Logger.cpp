#ifdef DEBUG_MODE
#include "Logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>


static constexpr size_t LOG_MSG_SIZE = 128;
static constexpr size_t LOG_POOL_SIZE = 32;

static QueueHandle_t _log_queue = nullptr;
static QueueHandle_t _free_queue = nullptr;
static bool _serial_started = false;
static portMUX_TYPE _log_lock = portMUX_INITIALIZER_UNLOCKED;
static char _log_buffers[LOG_POOL_SIZE][LOG_MSG_SIZE];

// Task สำหรับดึงข้อมูลจาก Queue มาออก Serial
static void _serial_log_task(void *pvParameters) {
  char *msg = nullptr;
  while (true) {
    if (xQueueReceive(_log_queue, &msg, portMAX_DELAY) == pdPASS) {
      if (msg) {
        Serial.print(msg);
        (void)xQueueSend(_free_queue, &msg, 0);
        msg = nullptr;
      }
    }
  }
}

// ฟังก์ชันเริ่มต้นภายใน (Auto-init)
static void _logger_auto_init() {
  if (_log_queue)
    return;

  portENTER_CRITICAL(&_log_lock);
  if (_log_queue) {
    portEXIT_CRITICAL(&_log_lock);
    return;
  }

  if (!_serial_started) {
    Serial.begin(115200);
    _serial_started = true;
  }

  _log_queue = xQueueCreate(LOG_POOL_SIZE, sizeof(char *));
  _free_queue = xQueueCreate(LOG_POOL_SIZE, sizeof(char *));
  if (_log_queue && _free_queue) {
    for (size_t i = 0; i < LOG_POOL_SIZE; i++) {
      char *buf = _log_buffers[i];
      (void)xQueueSend(_free_queue, &buf, 0);
    }
    xTaskCreatePinnedToCore(_serial_log_task, "LogTask", 4096, NULL, 1, NULL,
                            1);
  }

  portEXIT_CRITICAL(&_log_lock);
}

static void _log_send(const char *fmt, bool newline, va_list args) {
  if (!_log_queue)
    _logger_auto_init();
  if (!_log_queue || !_free_queue)
    return;

  char *buf = nullptr;
  if (xQueueReceive(_free_queue, &buf, 0) != pdPASS || !buf)
    return;

  int len = vsnprintf(buf, LOG_MSG_SIZE, fmt, args);
  if (len < 0) {
    (void)xQueueSend(_free_queue, &buf, 0);
    return;
  }

  if (newline) {
    if (len < (int)LOG_MSG_SIZE - 1) {
      buf[len] = '\n';
      buf[len + 1] = '\0';
    } else {
      buf[LOG_MSG_SIZE - 2] = '\n';
      buf[LOG_MSG_SIZE - 1] = '\0';
    }
  }

  if (xQueueSend(_log_queue, &buf, 0) != pdPASS) {
    (void)xQueueSend(_free_queue, &buf, 0);
  }
}

void log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _log_send(fmt, true, args);
  va_end(args);
}

void log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _log_send(fmt, false, args);
  va_end(args);
}
#endif
