# NK77 Library (ESP32) – ภาษาไทย

ไลบรารีสำหรับรับสัญญาณ pulse จากตัวรับธนบัตร NK77 และแปลงเป็นมูลค่าเครดิตด้วย callback

## คุณสมบัติ
- รองรับ callback: ตรวจจับเริ่มรับ, รับสำเร็จ, ถูกปฏิเสธ
- มีระบบคาลิเบรตอัตโนมัติ + fallback ตาม DIP
- ปรับจำนวน pulse ต่อ 10 บาทได้ผ่าน config

## การตั้งค่า (Config)
ไฟล์: `lib/nk77/src/nk77_config.h`

```cpp
// จำนวน pulse ต่อ 10 บาทตาม DIP (1,2,5,10)
#define NK77_PULSE_PER_10 2

// เกณฑ์ขั้นต่ำให้ถือว่าเริ่มรับ pulse แล้ว
#define MIN_VALID_PULSE 3
```

## API ที่ใช้บ่อย
ไฟล์: `lib/nk77/src/nk77.h`

- `void nk77::init(int pulsePin);`
- `void nk77::init(int pulsePin, int inhibitPin);`
- `void nk77::init(const nk77::InitConfig &config);`
- `void nk77::setInhibit(bool enable);`  
  `true` = บล็อกการรับแบงค์
- `void nk77::factoryResetCalibration();`  
  ล้างคาลิเบรตเดิม
- `void nk77::onCreditDetected(CreditDetectedCallback cb);`
- `void nk77::onCreditComplete(CreditCompleteCallback cb);`
- `void nk77::onCreditRejected(CreditRejectCallback cb);`
- `int nk77::getLastPulseCount();`  
  จำนวน pulse ล่าสุดที่ถูกสรุป (หลังรับเสร็จ)

## ตัวอย่างใช้งานแบบพื้นฐาน
```cpp
#include <Arduino.h>
#include "nk77.h"

void setup() {
  Serial.begin(115200);

  nk77::onCreditDetected([]() {
    Serial.println("Credit detected");
  });

  nk77::onCreditComplete([](uint32_t bill) {
    Serial.printf("pulse: %d\n", nk77::getLastPulseCount());
    Serial.printf("Credit complete: %u\n", bill);
  });

  nk77::onCreditRejected([](uint32_t pulse) {
    Serial.printf("Credit rejected: %u\n", pulse);
  });

  nk77::init(27, 16); // pulsePin, inhibitPin
  nk77::setInhibit(false);
}

void loop() {
  // main application
}
```

## หมายเหตุสำคัญ
- `getLastPulseCount()` จะมีค่าเมื่อการรับ pulse จบแล้วเท่านั้น  
  ถ้าพิมพ์ใน `Credit detected` จะยังเป็น 0 ได้
- ถ้าใช้ DIP ตั้งค่า pulse/10 บาท ให้แก้ `NK77_PULSE_PER_10` ให้ตรง
