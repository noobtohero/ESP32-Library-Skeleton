#include "nk77_calibration.h"
#include "nk77_config.h"
#include "nk77.h"

#include <math.h>
#include <string.h>

// -------- Statistics --------
struct BillStat
{
    uint32_t count;
    float mean;
    float var;
};

static BillStat stat20;
static BillStat stat50;
static BillStat stat100;

volatile uint32_t _flagPulseCount;

// -------- Welford update --------
static void updateStat(BillStat &s, uint32_t x)
{
    s.count++;
    float d = x - s.mean;
    s.mean += d / s.count;
    s.var += d * (x - s.mean);
}

// -------- Range check --------
static bool inRange(const BillStat &s, uint32_t x)
{
    if (s.count < CALIB_MIN_SAMPLE)
        return false;

    float sigma = sqrtf(s.var / s.count);
    return (x >= (s.mean - CALIB_SIGMA * sigma) &&
            x <= (s.mean + CALIB_SIGMA * sigma));
}

// -------- Decode + auto-cal --------
bool nk77_processPulse(uint32_t pulse)
{
    _flagPulseCount = pulse;
    int bill = 0;

    // ---- calibrated range ----
    if (inRange(stat20, pulse))
        bill = 20;
    else if (inRange(stat50, pulse))
        bill = 50;
    else if (inRange(stat100, pulse))
        bill = 100;

    // ---- cold start fallback ----
    
    if (bill == 0)
    {
        const int p10 = NK77_PULSE_PER_10;
        const int p20 = p10 * 2;
        const int p50 = p10 * 5;
        const int p100 = p10 * 10;

        // allow small tolerance around expected pulses
        if (pulse >= p20 - 1 && pulse <= p20 + 1)
            bill = 20;
        else if (pulse >= p50 - 2 && pulse <= p50 + 2)
            bill = 50;
        else if (pulse >= p100 - 4 && pulse <= p100 + 4)
            bill = 100;
    }

    // ---- reject ----
    if (bill == 0)
    {
        if (nk77::creditRejectCb)
            nk77::creditRejectCb(pulse);

        return false;
    }

    // ---- update calibration ----
    if (bill == 20)
        updateStat(stat20, pulse);
    else if (bill == 50)
        updateStat(stat50, pulse);
    else if (bill == 100)
        updateStat(stat100, pulse);

    // ---- accept ----
    if (nk77::creditCompleteCb)
        nk77::creditCompleteCb(bill);

    return true;
}

// -------- Factory reset --------
void nk77_factoryReset()
{
    memset(&stat20, 0, sizeof(stat20));
    memset(&stat50, 0, sizeof(stat50));
    memset(&stat100, 0, sizeof(stat100));
}


int getLastPulseCount()
{
    return _flagPulseCount;
}
