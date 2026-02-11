#include <Arduino.h>
#include "nk77.h"
#include "macro.h"
#include "Logger.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "utils/lcd/lcd_utils.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD I2C address

#define wait(x) vTaskDelay((x) / portTICK_PERIOD_MS);

// ===========================================================
// ##### User Settings #####
// ===========================================================
#define minPerBaht 2      // config by dip switch on NK77
#define multiplyBill true // true = à¸£à¸±à¸šà¸šà¸´à¸¥à¹„à¸”à¹‰à¸«à¸¥à¸²à¸¢à¹ƒà¸šà¸•à¹ˆà¸­à¹€à¸™à¸·à¹ˆà¸­à¸‡, false = à¸£à¸±à¸šà¸šà¸´à¸¥à¹„à¸”à¹‰à¸—à¸µà¸¥à¹ƒà¸š
#define MACHINE_NAME "  WIWA MASSAGE  "
#define VERSION "2.0.0"
#define MASSAGE_MODE 2

// ===========================================================
// ##### Time Constants #####
// ===========================================================
static const TickType_t ONE_SEC_TICKS = pdMS_TO_TICKS(1000);
static const uint32_t START_COUNTDOWN_SEC = 10;
static const uint32_t MACHINE_START_SEC = 40U * 60U;
static const uint32_t MACHINE_ADD_THRESHOLD_SEC = 30U * 60U;
static const uint32_t MACHINE_ADD_SEC = 10U * 60U;

// ===========================================================
// ##### State Machine #####
// ===========================================================
enum STATE
{
    RESET,
    IDLE,
    START,
    MASSAGE,
    END
};
static STATE currentState;

// --- massage time management ---
static portMUX_TYPE massageMux = portMUX_INITIALIZER_UNLOCKED; // à¹€à¸‚à¸µà¸¢à¸™à¸­à¹ˆà¸²à¸™à¸‚à¹‰à¸²à¸¡ core
static uint32_t massageSeconds = 0;

// --- start countdown ---
static TickType_t startLastTick = 0;
static uint32_t startRemainingSec = 0;
static bool startInit = false;

// --- machine massage time ---
static uint32_t machineRemainingSec = 0;
static TickType_t machineLastTick = 0;
static bool machineCountdownActive = false;

// ===========================================================
// ##### Forward Declarations #####
// ===========================================================
static void mainTask(void *arg);

static void onCreditDetected();
static void onCreditComplete(uint32_t bill);
static void onCreditRejected(uint32_t pulse);

static void convertBahtToMassageSec(uint32_t baht);
static uint32_t getMassageTime();
static uint32_t addMassageTime(uint32_t sec);
static uint32_t decMassageTime();
static STATE getState();
static void setState(STATE state);

void setup()
{
    Serial.begin(115200);
    log("-- setup started --");

    // setup NK77
    nk77::onCreditDetected(onCreditDetected);
    nk77::onCreditComplete(onCreditComplete);
    nk77::onCreditRejected(onCreditRejected);

    nk77::init(27, 16);
#ifdef DEBUG_MODE
    nk77::factoryResetCalibration();
#endif

    // setup LCD
    Wire.begin();
    lcd.init();
    lcd.backlight();
    LCD::init(&lcd);

    // setup Remote
    m_remote_init();
    acPower(true);

    setState(RESET);

    // create main task
    xTaskCreatePinnedToCore(
        mainTask,
        "mainTask",
        4096,
        NULL,
        1,
        NULL,
        1);

    // final setup
    log("-- setup completed --");
}

void loop()
{
    // do Nothing
    wait(1000);
}

// ===========================================================
// ##### Main Task #####
// ===========================================================
static void mainTask(void *arg)
{
    TickType_t lastTick = 0;

    for (;;)
    {
        TickType_t now = xTaskGetTickCount();
        STATE state = getState();

        switch (state)
        {
        case RESET:
            log("System Resetting...");
            LCD::print(VERSION, 0);
            LCD::print("System Resetting", 1);

            m_hardreset();
            log("System Idle");
            setState(IDLE);
            break;
        case IDLE:
            nk77::setInhibit(false); // reset inhibit to allow credit
            LCD::print(MACHINE_NAME, 0);
            LCD::print(" Insert Credit. ", 1);
            break;
        case START:
        {
            if (!startInit)
            {
                startInit = true;
                startRemainingSec = START_COUNTDOWN_SEC;
                startLastTick = now;
                log("Massage Started");
                LCD::print("  Massage On!  ", 0);
            }

            while (now - startLastTick >= ONE_SEC_TICKS)
            {
                startLastTick += ONE_SEC_TICKS;
                if (startRemainingSec > 0)
                {
                    startRemainingSec--;
                    log("Start remaining: %lu sec", static_cast<unsigned long>(startRemainingSec));
                    LCD::printTimeLabel("Start:", startRemainingSec, 1);
                }
            }

            if (startRemainingSec == 0)
            {
                startInit = false;
                LCD::print(" Body Scanning. ", 1);
                m_start(MASSAGE_MODE);
                machineRemainingSec = MACHINE_START_SEC;
                machineLastTick = xTaskGetTickCount();
                machineCountdownActive = true;
                setState(MASSAGE);
            }
            break;
        }
        case MASSAGE:
            LCD::print(MACHINE_NAME, 0);
            LCD::printTime(getMassageTime(), 1);

            if (now - lastTick >= ONE_SEC_TICKS)
            {
                lastTick = now;
                if (decMassageTime() == 0)
                {
                    setState(END);
                }
            }

            if (machineCountdownActive && (now - machineLastTick) >= ONE_SEC_TICKS)
            {
                machineLastTick += ONE_SEC_TICKS;
                if (machineRemainingSec > 0)
                {
                    machineRemainingSec--;
                    // When machine time reaches 30 min and user still has >= 30 min, add 10 min.
                    if (machineRemainingSec == MACHINE_ADD_THRESHOLD_SEC &&
                        getMassageTime() >= MACHINE_ADD_THRESHOLD_SEC)
                    {
                        log("Add machine time: +10 min");
                        m_addTimes(1);
                        machineRemainingSec += MACHINE_ADD_SEC;
                    }
                }
            }
            break;
        case END:
            log("Massage Ended");
            LCD::print("   Thank You!  ", 1);

            m_end();
            setState(IDLE);
            break;
        }

        wait(20)
    }
}

static void onCreditDetected()
{
    log("Credit detected");
}

static void onCreditComplete(uint32_t bill)
{
    if (!multiplyBill)
    {
        nk77::setInhibit(true); // single recive bill
    }

    // show credit on LCD
    String msg = " Credit: " + String(bill) + " Baht ";
    LCD::print(msg.c_str(), 1, 3, true);

    log("Credit complete: %lu baht", static_cast<unsigned long>(bill));
    convertBahtToMassageSec(bill);
}

static void onCreditRejected(uint32_t pulse)
{
    log("Credit rejected: %lu", static_cast<unsigned long>(pulse));
}

static void convertBahtToMassageSec(uint32_t baht)
{
    // minPerBaht is minutes per baht
    uint32_t sec = (baht / minPerBaht) * 60U;
    addMassageTime(sec);
    log("Added massage time: %lu sec", static_cast<unsigned long>(sec));
}

static uint32_t getMassageTime()
{
    uint32_t sec;
    portENTER_CRITICAL(&massageMux);
    sec = massageSeconds;
    portEXIT_CRITICAL(&massageMux);

    return sec;
}

static uint32_t addMassageTime(uint32_t sec)
{
    uint32_t _massageSeconds = 0;

    if (sec > 0)
    {
        // protect shared state on multi-core
        portENTER_CRITICAL(&massageMux);
        massageSeconds += sec;
        _massageSeconds = massageSeconds;
        if (currentState == IDLE)
        {
            currentState = START;
        }
        portEXIT_CRITICAL(&massageMux);
    }

    return _massageSeconds;
}

// decrease massage time by 1 second, return remaining time
static uint32_t decMassageTime()
{
    uint32_t _massageSeconds = 0;

    portENTER_CRITICAL(&massageMux);
    if (massageSeconds > 0)
    {
        massageSeconds--;
        _massageSeconds = massageSeconds;
    }
    portEXIT_CRITICAL(&massageMux);

    return _massageSeconds;
}

static STATE getState()
{
    STATE state;
    portENTER_CRITICAL(&massageMux);
    state = currentState;
    portEXIT_CRITICAL(&massageMux);
    return state;
}

static void setState(STATE state)
{
    portENTER_CRITICAL(&massageMux);
    currentState = state;
    portEXIT_CRITICAL(&massageMux);
}
