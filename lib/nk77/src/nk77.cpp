#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "nk77.h"
#include "nk77_config.h"
#include "nk77_calibration.h"

static QueueHandle_t pulseQueue;
nk77::CreditDetectedCallback nk77::creditDetectedCb = nullptr;
nk77::CreditCompleteCallback nk77::creditCompleteCb = nullptr;
nk77::CreditRejectCallback nk77::creditRejectCb = nullptr;

static int _pulsePin = -1;
static int _inhibitPin = -1;
static bool _activeLow = true;

// ---------------- ISR ----------------
static void IRAM_ATTR onPulseISR()
{
    uint8_t evt = 1;
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR(pulseQueue, &evt, &hp);
    if (hp)
        portYIELD_FROM_ISR();
}

// ---------------- Pulse Task ----------------
static void pulseTask(void *arg)
{
    uint8_t evt;
    uint32_t pulseCount = 0;
    TickType_t lastTick = 0;

    enum
    {
        IDLE,
        CANDIDATE,
        RECEIVING
    } state = IDLE;

    const TickType_t gapTick = pdMS_TO_TICKS(MIN_PULSE_GAP_MS);
    const TickType_t toutTick = pdMS_TO_TICKS(PULSE_TIMEOUT_MS);
    const uint32_t maxExpectedPulse = (NK77_PULSE_PER_10 * 10) + NK77_PULSE_TOL;

    for (;;)
    {
        if (xQueueReceive(pulseQueue, &evt, toutTick))
        {
            TickType_t now = xTaskGetTickCount();

            // debounce / noise gap
            if (pulseCount && (now - lastTick) < gapTick)
                continue;

            lastTick = now;
            pulseCount++;

            if (pulseCount > MAX_PULSE_LIMIT)
            {
                if (nk77::creditRejectCb)
                {
                    nk77::creditRejectCb(pulseCount);
                }

                goto reset;
            }

            if (state == IDLE)
                state = CANDIDATE;

            if (state == CANDIDATE &&
                pulseCount >= MIN_VALID_PULSE)
            {
                state = RECEIVING;
                // callback for credit detected
                if (nk77::creditDetectedCb)
                {
                    nk77::creditDetectedCb();
                }
            }

            // early complete when reached expected max pulses
            if (state == RECEIVING && pulseCount >= maxExpectedPulse)
            {
                nk77_processPulse(pulseCount);
                goto reset;
            }
        }
        else
        {
            // timeout
            if (state == RECEIVING)
            {
                nk77_processPulse(pulseCount);
            }
            else if (state == CANDIDATE)
            {
                if (nk77::creditRejectCb)
                {
                    nk77::creditRejectCb(pulseCount);
                }
            }

        reset:
            pulseCount = 0;
            state = IDLE;
        }
    }
}

// ---------------- Public API ----------------
void nk77::init(int pulsePin)
{
    InitConfig cfg;
    cfg.pulsePin = pulsePin;
    cfg.inhibitPin = -1;
    cfg.activeLow = NK77_ACTIVE_LOW;
    init(cfg);
}

void nk77::init(int pulsePin, int inhibitPin)
{
    InitConfig cfg;
    cfg.pulsePin = pulsePin;
    cfg.inhibitPin = inhibitPin;
    cfg.activeLow = NK77_ACTIVE_LOW;
    init(cfg);
}

void nk77::init(const InitConfig &cfg)
{
    _pulsePin = cfg.pulsePin;
    _inhibitPin = cfg.inhibitPin;
    _activeLow = cfg.activeLow;

    pulseQueue = xQueueCreate(32, sizeof(uint8_t));

    // setup pulse pin
    pinMode(_pulsePin, INPUT_PULLUP);
    attachInterrupt(
        _pulsePin,
        onPulseISR,
        _activeLow ? FALLING : RISING);

    // setup inhibit pin
    if (_inhibitPin >= 0)
    {
        pinMode(_inhibitPin, OUTPUT);
        digitalWrite(_inhibitPin, HIGH); // default disable
    }

    xTaskCreatePinnedToCore(
        pulseTask,
        "nk77Pulse",
        8000,
        nullptr,
        10,
        nullptr,
        1);
}

void nk77::factoryResetCalibration()
{
    nk77_factoryReset();
}

void nk77::setInhibit(bool enable)
{
    if (_inhibitPin < 0) return;
    digitalWrite(_inhibitPin, enable ? LOW : HIGH);
}

void nk77::onCreditDetected(CreditDetectedCallback cb)
{
    creditDetectedCb = cb;
}

void nk77::onCreditComplete(CreditCompleteCallback cb)
{
    creditCompleteCb = cb;
}

void nk77::onCreditRejected(CreditRejectCallback cb)
{
    creditRejectCb = cb;
}

int nk77::getLastPulseCount()
{
    return ::getLastPulseCount();
}
