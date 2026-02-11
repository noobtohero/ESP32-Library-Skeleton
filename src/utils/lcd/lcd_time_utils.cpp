#include "lcd_utils.h"
#include <stdio.h>
#include <string.h>

void secToMMSS(uint32_t sec, char *out, size_t outSize)
{
    if (out == NULL || outSize < 6)
    {
        return;
    }

    uint32_t min = sec / 60;
    uint32_t s = sec % 60;

    snprintf(out, outSize, "%02lu:%02lu", (unsigned long)min, (unsigned long)s);
}

const char *secToMMSS(uint32_t sec)
{
    static char buf[6];
    secToMMSS(sec, buf, sizeof(buf));
    return buf;
}

namespace
{
    void centerLine(const char *src, char *dst, size_t dstSize)
    {
        if (src == NULL || dst == NULL || dstSize == 0)
        {
            return;
        }

        size_t len = strlen(src);
        if (len >= LCD_COLS)
        {
            strncpy(dst, src, LCD_COLS);
            dst[LCD_COLS] = '\0';
            return;
        }

        size_t padLeft = (LCD_COLS - len) / 2;
        size_t i = 0;
        for (; i < padLeft && i < dstSize - 1; i++)
        {
            dst[i] = ' ';
        }

        size_t j = 0;
        while (j < len && i < dstSize - 1)
        {
            dst[i++] = src[j++];
        }

        while (i < LCD_COLS && i < dstSize - 1)
        {
            dst[i++] = ' ';
        }
        dst[i] = '\0';
    }
}

namespace LCD
{
    bool printTime(uint32_t sec, uint8_t row)
    {
        char line[LCD_MAX_MSG_LEN] = {0};
        centerLine(secToMMSS(sec), line, sizeof(line));
        return print(line, row);
    }

    bool printTimeLabel(const char *label, uint32_t sec, uint8_t row)
    {
        if (label == NULL)
        {
            return false;
        }

        char raw[LCD_MAX_MSG_LEN] = {0};
        snprintf(raw, sizeof(raw), "%s %s", label, secToMMSS(sec));

        char line[LCD_MAX_MSG_LEN] = {0};
        centerLine(raw, line, sizeof(line));
        return print(line, row);
    }
}
