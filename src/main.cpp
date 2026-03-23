#include <Arduino.h>
#include "nk77.h"
#include "macro.h"
#include "Logger.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "utils/lcd/lcd_utils.h"
#include "config/task_stack_config.h"
#include "utils/task_stack/task_stack_monitor.h"
#include <freertos/semphr.h>
#include <WiFi.h>
#include <SimpleOTA.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD I2C address

#define wait(x) vTaskDelay((x) / portTICK_PERIOD_MS);

// ===========================================================
// ##### User Settings #####
// ===========================================================
#define minPerBaht 2      // config by dip switch on NK77
#define multiplyBill true // true = accept multiple bills, false = single bill only
#define MACHINE_NAME "  WIWA MASSAGE  "
#define VERSION "2.0.0"
#define MASSAGE_MODE 2
#define MAXIMUM_CREDIT 120 // maximum credit in baht
// ===========================================================

// ##### WiFi Settings #####
#define SSID "Wiwamassage"
#define PWD "26462646"
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // 15 sec

// ===========================================================
// ##### Time Constants #####
// ===========================================================
static const TickType_t ONE_SEC_TICKS = pdMS_TO_TICKS(1000); // 1 วินาทีใน FreeRTOS
static const uint32_t START_COUNTDOWN_SEC = 10;              // นับถอยหลังก่อนเริ่มนวด ให้ลูกค้าเตรียมตัว
static const uint32_t MACHINE_START_SEC = 40U * 60U;         // เวลาเครื่องเริ่มต้น 40 นาที
static const uint32_t MACHINE_ADD_THRESHOLD_SEC = 30U * 60U; // เมื่อเวลาเครื่องเหลือ 29 นาที
static const uint32_t MACHINE_ADD_SEC = 10U * 60U;           // เพิ่มเวลาเครื่องครั้งละ 10 นาที

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

// --- remote task helpers ---
static SemaphoreHandle_t remoteMutex = NULL;

// ===========================================================
// ##### Forward Declarations #####
// ===========================================================
static void mainTask(void *arg);
static void addMachineTime(uint8_t times);
static void lockRemote();
static void unlockRemote();
static void resetToIdle();

static void onCreditDetected();
static void onCreditComplete(uint32_t bill);
static void onCreditRejected(uint32_t pulse);

static uint32_t summaryCredit = 0;
static void convertBahtToMassageSec(uint32_t baht);
static uint32_t getMassageTime();
static uint32_t addMassageTime(uint32_t sec);
static uint32_t decMassageTime();
static uint32_t getStartRemainingSec();
static uint32_t addStartRemainingSec(uint32_t sec);
static bool tryDecStartRemainingSec(uint32_t &remainingSec);
static STATE getState();
static void setState(STATE state);

void setup()
{
    Serial.begin(115200);
    log("-- setup started --");

    // WiFi.begin(SSID, PWD);
    // log_raw("WiFi Connecting");
    // const unsigned long wifiConnectStart = millis();
    // while (WiFi.status() != WL_CONNECTED &&
    //        (millis() - wifiConnectStart) < WIFI_CONNECT_TIMEOUT_MS)
    // {
    //     delay(500);
    //     log_raw(".");
    // }

    // if (WiFi.status() == WL_CONNECTED)
    // {
    //     log("WiFi Connected");
    //     // Setup OTA

    //     OTA.setAuth("admin", "admin1234");
    //     OTA.setManifestUrl("http://esp32.noobtohero.com/ota/manifest.json");
    //     OTA.begin("2.0.0");
    // }
    // else
    // {
    //     log("WiFi connect timeout after %lu ms", static_cast<unsigned long>(WIFI_CONNECT_TIMEOUT_MS));
    // }

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
    acPower(false);
    remoteMutex = xSemaphoreCreateMutex();

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
    TickType_t stackLogTick = 0;

    for (;;)
    {
        TickType_t now = xTaskGetTickCount();
        STATE state = getState();

        switch (state)
        {
        case RESET:
            nk77::setInhibit(true); // block credit during reset
            log("System Resetting...");
            LCD::print(VERSION, 0);
            LCD::print("System Resetting", 1);

            lockRemote();
            m_hardreset();
            unlockRemote();
            log("System Idle");
            resetToIdle();
            break;
        case IDLE:

            break;
        case START:
        {

            if (!startInit)
            {
                startInit = true;
                startLastTick = now;
                log("Massage Started");
                LCD::print("  Massage On!  ", 0);
            }

            while (now - startLastTick >= ONE_SEC_TICKS)
            {
                uint32_t remainingSec = 0;
                startLastTick += ONE_SEC_TICKS;
                if (tryDecStartRemainingSec(remainingSec))
                {
                    log("Start remaining: %lu sec", static_cast<unsigned long>(remainingSec));
                    LCD::printTimeLabel("Start:", remainingSec, 1);
                }
            }

            if (getStartRemainingSec() == 0)
            {
                nk77::setInhibit(true);
                startInit = false;
                LCD::print(" Body Scanning. ", 1);
                lockRemote();
                m_start(MASSAGE_MODE);
                unlockRemote();
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
                        addMachineTime(1);
                        machineRemainingSec += MACHINE_ADD_SEC;
                    }
                }
            }
            break;
        case END:
            log("Massage Ended");
            LCD::print("   Thank You!  ", 1);

            lockRemote();
            m_end();
            unlockRemote();
            resetToIdle();
            break;
        }

        // TaskStackMonitor::logStackUsageOnce(
        //     now,
        //     stackLogTick,
        //     TaskStackConfig::kMonitorPeriodTicks,
        //     TaskStackConfig::kTasks,
        //     TaskStackConfig::kTaskCount,
        //     TaskStackConfig::kWarnFreeBytes);
        wait(20)
    }
}

static void onCreditDetected()
{
    log("Credit detected");
}

static void onCreditComplete(uint32_t bill)
{
    summaryCredit += bill;
    if (!multiplyBill || (summaryCredit >= MAXIMUM_CREDIT))
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
    // inclede startRemainingSec = START_COUNTDOWN_SEC ต่อเวลานับถอยหลังเมื่อใส่แบงค์เพิ่ม
    // แต่งต้อง block startRemainingSec ป้องกัน race condition กับ mainTask
    addStartRemainingSec(START_COUNTDOWN_SEC);

    // minPerBaht is minutes per baht
    uint32_t sec = ((baht / minPerBaht) * 60U) + 1; // +1 sec to avoid zero time
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

static uint32_t getStartRemainingSec()
{
    uint32_t sec;
    portENTER_CRITICAL(&massageMux);
    sec = startRemainingSec;
    portEXIT_CRITICAL(&massageMux);
    return sec;
}

static uint32_t addStartRemainingSec(uint32_t sec)
{
    uint32_t remainingSec;
    portENTER_CRITICAL(&massageMux);
    if (startRemainingSec > START_COUNTDOWN_SEC)
    {
        startRemainingSec = START_COUNTDOWN_SEC;
    }

    if (sec > 0 && startRemainingSec < START_COUNTDOWN_SEC)
    {
        uint32_t freeSec = START_COUNTDOWN_SEC - startRemainingSec;
        startRemainingSec += (sec < freeSec) ? sec : freeSec;
    }

    remainingSec = startRemainingSec;
    portEXIT_CRITICAL(&massageMux);
    return remainingSec;
}

static bool tryDecStartRemainingSec(uint32_t &remainingSec)
{
    bool decremented = false;
    portENTER_CRITICAL(&massageMux);
    if (startRemainingSec > 0)
    {
        startRemainingSec--;
        decremented = true;
    }
    remainingSec = startRemainingSec;
    portEXIT_CRITICAL(&massageMux);
    return decremented;
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

// ===========================================================
// ##### Remote One-shot Task #####
// ===========================================================
static void macroAddTimesTask(void *arg)
{
    uint8_t times = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(arg));
    if (times > 0)
    {
        lockRemote();
        m_addTimes(times);
        unlockRemote();
    }
    vTaskDelete(NULL);
}

static void addMachineTime(uint8_t times)
{
    if (times == 0)
    {
        return;
    }

    xTaskCreatePinnedToCore(
        macroAddTimesTask,
        "macroAddTimes",
        4096,
        reinterpret_cast<void *>(static_cast<uintptr_t>(times)),
        1,
        NULL,
        1);
}

static void lockRemote()
{
    if (remoteMutex != NULL)
    {
        xSemaphoreTake(remoteMutex, portMAX_DELAY);
    }
}

static void unlockRemote()
{
    if (remoteMutex != NULL)
    {
        xSemaphoreGive(remoteMutex);
    }
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

static void resetToIdle()
{
    // Reset shared state for a clean new session.
    portENTER_CRITICAL(&massageMux);
    massageSeconds = 0;
    startRemainingSec = 0;
    startInit = false;
    machineRemainingSec = 0;
    machineLastTick = 0;
    machineCountdownActive = false;
    summaryCredit = 0;
    portEXIT_CRITICAL(&massageMux);

    nk77::setInhibit(false);
    LCD::print(MACHINE_NAME, 0);
    LCD::print(" Insert Credit. ", 1);
    setState(IDLE);
}
