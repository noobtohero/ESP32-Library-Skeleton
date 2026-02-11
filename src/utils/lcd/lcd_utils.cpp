#include "lcd_utils.h"

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ================================
// internal types
// ================================
typedef enum
{
    LCD_CMD_PRINT,
    LCD_CMD_CLEAR_ROW,
    LCD_CMD_CLEAR_ALL
} LcdCommand;

typedef struct
{
    LcdCommand cmd;
    uint8_t row;
    char msg[LCD_MAX_MSG_LEN];
    uint32_t holdMs;
    bool autoClear;
} LcdMessage;

// ================================
// internal state
// ================================
static LiquidCrystal_I2C *lcdDev = NULL;
static QueueHandle_t lcdQueue = NULL;

static char lastLine[LCD_ROWS][LCD_MAX_MSG_LEN] = {"", ""};

// ================================
// lcd task
// ================================
static void lcdTask(void *pv)
{
    LcdMessage m;

    for (;;)
    {
        if (xQueueReceive(lcdQueue, &m, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        switch (m.cmd)
        {
        // ==========================
        case LCD_CMD_PRINT:
            // ==========================
            if (m.row >= LCD_ROWS)
            {
                break;
            }

            if (strcmp(lastLine[m.row], m.msg) == 0)
            {
                break; // duplicate → ignore
            }

            lcdDev->setCursor(0, m.row);
            for (uint8_t i = 0; i < LCD_COLS; i++)
            {
                lcdDev->print(' ');
            }

            lcdDev->setCursor(0, m.row);
            lcdDev->print(m.msg);

            strncpy(lastLine[m.row], m.msg, LCD_MAX_MSG_LEN);
            lastLine[m.row][LCD_MAX_MSG_LEN - 1] = '\0';

            if (m.holdMs > 0)
            {
                vTaskDelay(pdMS_TO_TICKS(m.holdMs));
                if (m.autoClear)
                {
                    lcdDev->setCursor(0, m.row);
                    for (uint8_t i = 0; i < LCD_COLS; i++)
                    {
                        lcdDev->print(' ');
                    }
                    lastLine[m.row][0] = '\0';
                }
            }
            break;

        // ==========================
        case LCD_CMD_CLEAR_ROW:
            // ==========================
            if (m.row >= LCD_ROWS)
            {
                break;
            }

            if (lastLine[m.row][0] == '\0')
            {
                break; // already clear
            }

            lcdDev->setCursor(0, m.row);
            for (uint8_t i = 0; i < LCD_COLS; i++)
            {
                lcdDev->print(' ');
            }

            lastLine[m.row][0] = '\0';
            break;

        // ==========================
        case LCD_CMD_CLEAR_ALL:
            // ==========================
            lcdDev->clear();

            for (uint8_t r = 0; r < LCD_ROWS; r++)
            {
                lastLine[r][0] = '\0';
            }
            break;
        }
    }
}

// ================================
// public API
// ================================
namespace LCD
{
    // initialize LCD utils
    bool init(LiquidCrystal_I2C *lcd)
    {
        if (lcd == NULL)
        {
            return false;
        }

        lcdDev = lcd;

        lcdQueue = xQueueCreate(6, sizeof(LcdMessage));
        if (lcdQueue == NULL)
        {
            return false;
        }

        xTaskCreatePinnedToCore(
            lcdTask,
            "lcdTask",
            3072,
            NULL,
            1,
            NULL,
            1);

        return true;
    }

    // print message to lcd
    bool print(const char *msg, uint8_t row = 0)
    {
        if (lcdQueue == NULL || msg == NULL)
        {
            return false;
        }

        if (row >= LCD_ROWS)
        {
            return false;
        }

        LcdMessage m;
        m.cmd = LCD_CMD_PRINT;
        m.row = row;
        m.holdMs = 0;
        m.autoClear = false;

        strncpy(m.msg, msg, LCD_MAX_MSG_LEN - 1);
        m.msg[LCD_MAX_MSG_LEN - 1] = '\0';

        // non-blocking send
        if (xQueueSend(lcdQueue, &m, 0) != pdPASS)
        {
            // queue full → drop message
            return false;
        }

        return true;
    }

    bool print(const char *msg, uint8_t row, uint32_t holdSec, bool autoClear)
    {
        if (lcdQueue == NULL || msg == NULL)
        {
            return false;
        }

        if (row >= LCD_ROWS)
        {
            return false;
        }

        LcdMessage m;
        m.cmd = LCD_CMD_PRINT;
        m.row = row;
        m.holdMs = holdSec * 1000U;
        m.autoClear = autoClear;

        strncpy(m.msg, msg, LCD_MAX_MSG_LEN - 1);
        m.msg[LCD_MAX_MSG_LEN - 1] = '\0';

        if (xQueueSend(lcdQueue, &m, 0) != pdPASS)
        {
            return false;
        }

        return true;
    }

    // clear specific row
    bool clear(uint8_t row)
    {
        if (lcdQueue == NULL || row >= LCD_ROWS)
        {
            return false;
        }

        LcdMessage m;
        m.cmd = LCD_CMD_CLEAR_ROW;
        m.row = row;

        return (xQueueSend(lcdQueue, &m, 0) == pdPASS);
    }

    // clear entire lcd
    bool clearAll()
    {
        if (lcdQueue == NULL || lcdDev == NULL)
        {
            return false;
        }

        LcdMessage m;
        m.cmd = LCD_CMD_CLEAR_ALL;
        m.row = 0;

        return (xQueueSend(lcdQueue, &m, pdMS_TO_TICKS(50)) == pdPASS);
    }
}
