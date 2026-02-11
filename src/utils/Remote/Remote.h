#ifndef REMOTE_H
#define REMOTE_H

#include <Arduino.h>
#include <PCF8574.h>
#include <Wire.h>

class Remote
{
public:
  Remote(int address = 0x20, int sda = 21, int scl = 22);

  void begin();
  void swOn(int count = 1);
  void swMenu(int count = 1);
  void swUp(int count = 1);
  void swDown(int count = 1);
  void swLeft(int count = 1);
  void swRight(int count = 1);
  void swOk(int count = 1);
  void setAcPower(bool sw);

  // helper
  void wait(int delayTimeMs);

private:
  PCF8574 _pcf;
  enum Button
  {
    on = 0,
    menu = 1,
    up = 2,
    down = 3,
    left = 4,
    right = 5,
    ok = 6,
    acPower = 7
  };
  void _click(Button button, int count = 1);
};

#endif // REMOTE_H
