#include <Arduino.h>
#include "nk77.h"

void setup()
{
    Serial.begin(115200);

    nk77::onCreditDetected([]()
                           { Serial.println("Credit detected"); });
    nk77::onCreditComplete([](uint32_t bill)
                           {
         nk77::setInhibit(true);
        Serial.print("Credit complete: ");
        Serial.println(bill);
      Serial.printf("pulse: %d\n",nk77::getLastPulseCount()); });
    nk77::onCreditRejected([](uint32_t pulse)
                           {
        Serial.print("Credit rejected: ");
        Serial.println(pulse); });

    nk77::init(27, 16);

    nk77::setInhibit(false);
    nk77::factoryResetCalibration();
}

void loop()
{
    // main application
}
