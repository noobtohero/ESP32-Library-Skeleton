#pragma once

#include <stdint.h>
#include <stddef.h>

class LiquidCrystal_I2C;

#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_MAX_MSG_LEN (LCD_COLS + 1)

namespace LCD
{
  bool init(LiquidCrystal_I2C *lcd);
  bool print(const char *msg, uint8_t row);
  bool print(const char *msg, uint8_t row, uint32_t holdSec, bool autoClear = false);
  bool clear(uint8_t row);
  bool clearAll();
  bool printTime(uint32_t sec, uint8_t row);
  bool printTimeLabel(const char *label, uint32_t sec, uint8_t row);
}

void secToMMSS(uint32_t sec, char *out, size_t outSize);
const char *secToMMSS(uint32_t sec);
