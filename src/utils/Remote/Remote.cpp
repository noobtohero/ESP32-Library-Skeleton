#include "Remote.h"

Remote::Remote(int address, int sda, int scl) : _pcf(address, sda, scl) {}

void Remote::begin() {
  for (int i = 0; i < 8; i++) {
    _pcf.pinMode(i, OUTPUT, 1);
  }
  _pcf.begin();
}

void Remote::wait(int delayTimeMs) { vTaskDelay(pdMS_TO_TICKS(delayTimeMs)); }

void Remote::_click(Button button, int count) {
  while (count > 0) {
    _pcf.digitalWrite((int)button, 0);
    wait(650);
    _pcf.digitalWrite((int)button, 1);
    wait(650);
    count--;
  }
}

void Remote::swOn(int count) { _click(on, count); }
void Remote::swMenu(int count) { _click(menu, count); }
void Remote::swUp(int count) { _click(up, count); }
void Remote::swDown(int count) { _click(down, count); }
void Remote::swLeft(int count) { _click(left, count); }
void Remote::swRight(int count) { _click(right, count); }
void Remote::swOk(int count) { _click(ok, count); }

void Remote::setAcPower(bool sw) {
  // SSR active low
  _pcf.digitalWrite((int)acPower, !sw);
  wait(4000); // !Important หน่วงเวลาเพื่อให้เครื่องนวดเคลียร์ไฟค้าง
}
