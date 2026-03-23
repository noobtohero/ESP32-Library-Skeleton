# SimpleOTA Library

ห้องสมุดสำหรับทำ OTA (Over-The-Air) บน ESP32 แบบง่ายๆ เสียบปล๊กใช้ได้ทันที มาพร้อมกับ Web UI ที่สวยงามและทันสมัย

## คุณสมบัติ (Features)
- 🔌 **Plug & Play**: ใช้งานง่ายเพียงเรียกคำสั่ง `begin()`
- 🎨 **Web UI**: หน้าเว็บสำหรับอัปโหลด Firmware ดีไซน์สวยงาม (Dark Mode) ดูทันสมัย
- 🔒 **Security**: รองรับการตั้ง Username และ Password ก่อนเข้าหน้าอัปเดต
- ⚡ **No Dependencies**: ไม่ต้องลง Library ภายนอกเพิ่ม (ใช้ Standard ESP32 WebServer)
- 📱 **Responsive**: รองรับการแสดงผลบนมือถือ
- 🌐 **Online Update**: รองรับการอัปเดต Firmware ผ่าน URL (Manifest parsing)
- 📂 **File Update**: รองรับการอัปเดต Firmware จากไฟล์ใน FileSystem (LittleFS/SPIFFS)
- 🔄 **Auto Reboot**: รีบูตเครื่องอัตโนมัติเมื่ออัปเดตเสร็จสมบูรณ์

## การติดตั้ง (Installation)

### 1. แบบ Manual
1. นำโฟลเดอร์ `SimpleOTA` ไปวางในโฟลเดอร์ `lib` ของโปรเจกต์ PlatformIO หรือ `libraries` ของ Arduino
2. เพิ่ม `lib_ldf_mode = deep` ใน `platformio.ini` (สำหรับ PlatformIO)

### 2. ผ่าน GitHub (แนะนำสำหรับ PlatformIO)

หากคุณอัปโหลด Library นี้ขึ้น GitHub แล้ว สามารถเรียกใช้งานได้ทันทีโดยเพิ่มใน `platformio.ini`:

```ini
lib_deps =
    https://github.com/USERNAME/SimpleOTA.git
```

> [!TIP]
> อย่าลืมเปลี่ยน `USERNAME` เป็นชื่อผู้ใช้ของคุณ และเช็กว่าระดับโฟลเดอร์ใน Repo ตรงกับโครงสร้างของ Library นี้

## การใช้งานเบื้องต้น (Basic Usage)

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <SimpleOTA.h>

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASSWORD");
    while(WiFi.status() != WL_CONNECTED) delay(500);

    // ตั้งค่า OTA
    OTA.setAuth("admin", "1234");   // กำหนดรหัสผ่าน (ถ้าต้องการ)
 
    // เริ่มทำงาน
    OTA.begin("1.0.0"); 
}

void loop() {
    // ไม่ต้องเรียก OTA.loop() แล้ว เพราะทำงานแยก Task อัตโนมัติ
}
```

## เครื่องมือสำหรับนักพัฒนา (Developer Tools)
ในโฟลเดอร์ `lib/SimpleOTA/tools` จะมีสนาป `generate_assets.py`
- ใช้สำหรับแปลงไฟล์ `index.html`, `style.css`, `script.js` เป็น `webui.h`
- เมื่อแก้ไขไฟล์ใน `data/` เสร็จแล้ว ให้รันสคริปต์นี้เพื่ออัปเดต web interface
- **วิธีใช้**: `python lib/SimpleOTA/tools/generate_assets.py`

## 📑 Manifest File Format (`ota.txt`)
ไฟล์ Manifest เป็นไฟล์ข้อความธรรมดา (Text File) ที่ฝากไว้บน Server เพื่อระบุข้อมูลการอัปเดต

```text
version=1.1.0
url=https://your-server.com/firmware.bin
md5=5d41402abc4b2a76b9719d911017c592
```

- **`version`**: เวอร์ชันเป้าหมาย (เลือกใส่ได้ เพื่อใช้ตรวจสอบเวอร์ชัน)
- **`url`**: ลิงก์ตรงของไฟล์ `.bin` (รองรับทั้ง `http` และ `https`)
- **`md5`**: (ระบุได้) ค่า Hash สำหรับตรวจสอบความถูกต้องของไฟล์ก่อนการ Flash (แนะนำอย่างยิ่งสำหรับงานจริง)

## วิธีเข้าใช้งาน Web UI
1. เปิด Browser แล้วพิมพ์ `http://<IP_ESP32>/ota`
2. ล็อกอินด้วย Username/Password ที่ตั้งไว้
3. เลือกไฟล์ Firmware (`.bin`) แล้วกด Update

## API Reference

### การตั้งค่าและเริ่มต้น
- `void begin(const char* version, WebServer* server = nullptr)`: เริ่มการทำงาน สามารถระบุ custom server ได้
- `void setVersion(const char* version)`: ตั้งค่าเวอร์ชันปัจจุบันที่จะแสดงบนหน้าเว็บ
- `void setAuth(const char* username, const char* password)`: ตั้งค่ารหัสผ่านเข้าหน้าเว็บ
- `void useServer(WebServer* server)`: ใช้ WebServer จากภายนอกแทนตัว Built-in

### การอัปเดตแบบอื่นๆ
- `void fileUpdate(const char* path, const char* version = "")`: อัปเดตจากไฟล์ในเครื่อง (LittleFS)
- `void onlineUpdate(const char* manifestUrl)`: อัปเดตจาก URL (ต้องมีไฟล์ manifest)

### Callbacks (เหตุการณ์ต่างๆ)
- `void onUpdateStart(OTACallback cb)`: ทำงานเมื่อเริ่มอัปเดต
- `void onUpdateEnd(OTACallback cb)`: ทำงานเมื่ออัปเดตเสร็จ (ก่อนรีบูต)
- `void onUpdateProgress(OTAProgressCallback cb)`: ทำงานระหว่างอัปเดต (ส่งค่า progress, total)
- `void onUpdateError(OTAErrorCallback cb)`: ทำงานเมื่อเกิดข้อผิดพลาด

## 🔌 REST API Reference
คุณสามารถเชื่อมต่อ SimpleOTA กับ Dashboard ภายนอกได้ผ่าน JSON API:

- **GET `/ota/status`**: ดึงข้อมูลสถานะปัจจุบัน
  - คืนค่าเป็น JSON: `{"version":"...", "deviceID":"...", "freespace":..., "url":"...", "interval":..., "isUpdating":..., "progress":..., "lastError":"..."}`
- **POST `/ota/setAutoUpdate`**: ตั้งค่า URL และรอบการตรวจสอบ (Cloud OTA)
  - Body (form-data): `url` (String), `interval` (Hours) - กิิจกรรม 0 = ปิด
- **POST `/ota/setDeviceID`**: ตั้งชื่อเรียกอุปกรณ์ (Device ID)
  - Body (form-data): `deviceID` (String)
- **POST `/ota/fileUpdate`**: อัปโหลดไฟล์ Firmware โดยตรง (Manual OTA)
  - Body (form-data): `firmware` (File)
- **POST `/ota/rollback`**: สั่งทำ Rollback กลับไปเวอร์ชันก่อนหน้าทันที

## 🧩 Headless / API-Only Usage
หากคุณไม่ต้องการใช้หน้า Web UI หลัก และต้องการใช้เฉพาะ API เพื่อเชื่อมกับระบบของคุณเอง:

```cpp
// ส่ง pointer ของ WebServer ของคุณให้ SimpleOTA
OTA.begin("1.0.0", &myServer); 

// ตอนนี้ Web UI หลักจะเข้าไม่ได้ แต่ API /ota/status ยังคงทำงานอยู่
// คุณสามารถดึงข้อมูล JSON ไปแสดงผลบน Dashboard ของคุณเองได้เลย
```

---
*Developed by NTH*
