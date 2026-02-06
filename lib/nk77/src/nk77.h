#pragma once
#include <stdint.h>

namespace nk77
{
    struct InitConfig
    {
        int pulsePin;
        int inhibitPin; // -1 = not used
        bool activeLow;
    };

    // Callback types
    typedef void (*CreditDetectedCallback)();
    typedef void (*CreditCompleteCallback)(uint32_t baht);
    typedef void (*CreditRejectCallback)(uint32_t pulseCount);

    extern CreditDetectedCallback creditDetectedCb;
    extern CreditCompleteCallback creditCompleteCb;
    extern CreditRejectCallback creditRejectCb;

    void init(int pulsePin);
    void init(int pulsePin, int inhibitPin);
    void init(const InitConfig &config);

    void factoryResetCalibration();
    void setInhibit(bool enable); // true = block bill

    void onCreditDetected(CreditDetectedCallback cb);
    void onCreditComplete(CreditCompleteCallback cb);
    void onCreditRejected(CreditRejectCallback cb);

    // Expose last pulse count from calibration module
    int getLastPulseCount();
}
