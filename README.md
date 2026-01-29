# ESP32 Library Template Skeleton

โปรเจกต์ต้นแบบ (Skeleton Template) สำหรับการพัฒนาไลบรารี (Library) ของ ESP32 ด้วย PlatformIO โดยเน้นโครงสร้างที่สะอาด ทันสมัย และสอดคล้องกับมาตรฐาน Arduino/PlatformIO

## 📂 โครงสร้างโปรเจกต์ (Project Structure)

การจัดวางโฟลเดอร์ถูกออกแบบตามหลัก **Separation of Concerns** เพื่อให้ไลบรารีของคุณจัดการง่ายและนำไปใช้ต่อได้สะดวก:

```text
.
├── lib/
│   └── esp-library/            # โฟลเดอร์หลักของไลบรารี (เปลี่ยนชื่อตามต้องการ)
│       ├── data/               # ไฟล์เสริมอื่นๆ (เช่น Web assets, Default configs)
│       ├── docs/               # เอกสารประกอบการใช้งาน (Documentation)
│       ├── examples/           # ตัวอย่างการใช้งานสำหรับผู้ใช้
│       ├── src/                # Source code หลัก
│       │   ├── core/           # หัวใจหลักของ Logic (เช่น Update Engine, WiFi Manager)
│       │   ├── adaptor/        # ส่วนเชื่อมต่อภายนอก (เช่น Web UI, Route, Storage)
│       │   ├── utils/          # ฟังก์ชันช่วยงานทั่วไป (Helpers)
│       │   ├── esp-library.h   # Entry point หลักที่ผู้ใช้จะ #include
│       │   └── esp-library.cpp
│       ├── tools/              # สคริปต์หรือเครื่องมือช่วยพัฒนา (เช่น Python scripts)
│       ├── library.json        # ตั้งค่าสำหรับ PlatformIO Registry
│       └── library.properties  # ตั้งค่าสำหรับ Arduino IDE Library Manager
├── src/                        # พื้นที่สำหรับเขียน Sandbox/Test Code
│   └── main.cpp                # ไฟล์หลักสำหรับทดสอบเรียกใช้ไลบรารีใน lib/
├── .gitignore                  # ไฟล์ระบุสิ่งที่ Git ไม่ต้องติดตาม (เช่น build items)
└── platformio.ini              # การตั้งค่า Environment สำหรับพัฒนา (ESP32)
```

## 🚀 ฟีเจอร์ที่รวมไว้ (Features)

- **Clean Architecture**: แยกชั้นการทำงานชัดเจนระหว่าง Core Logic และ Adaptor (Interface)
- **High-Speed Development**: ตั้งค่า `upload_speed = 921600` เพื่อการคอมพายล์และอัปโหลดที่รวดเร็ว
- **Ready-to-Registry**: มีไฟล์ `library.json` ตัวอย่างพร้อมสำหรับการส่งขึ้น PlatformIO Registry
- **Sandbox Environment**: สามารถพัฒนาไลบรารีในโฟลเดอร์ `lib/` และทดสอบการใช้งานได้ทันทีใน `src/` โดยไม่ต้องติดตั้งใหม่ทุกครั้ง

## 🛠️ วิธีการนำไปใช้งาน (How to use)

1. **Clone/Copy**: คัดลอกโครงร่างนี้ไปยังโฟลเดอร์โปรเจกต์ใหม่ของคุณ
2. **Rename**: เปลี่ยนชื่อโฟลเดอร์ `lib/esp-library` ให้เป็นชื่อไลบรารีที่คุณต้องการพัฒนา
3. **Configure**: แก้ไขข้อมูลใน `lib/your-library/library.json` เช่น ชื่อผู้พัฒนา, เวอร์ชั่น และ Description
4. **Develop**:
    - เขียน Logic หลักไว้ใน `src/core/`
    - เขียนส่วนเชื่อมต่อผู้ใช้หรือ Web API ไว้ใน `src/adaptor/`
    - รวบรวม Header ไว้ที่ไฟล์หลัก (เช่น `your-library.h`)
5. **Test**: เรียกใช้งานไลบรารีของคุณใน `src/main.cpp` เพื่อทดสอบความถูกต้อง

## 📝 คำแนะนำในการพัฒนา

- ควรแยกไฟล์เป็น `.h` และ `.cpp` เพื่อความเร็วในการคอมพายล์ (Incremental Build)
- ตรวจสอบให้แน่ใจว่าโฟลเดอร์ `examples/` มีตัวอย่างการใช้งานอย่างน้อย 1 อย่างที่ทำงานได้จริง
- หากไลบรารีมีการใช้พื้นที่เยอะ แนะนำให้ใช้ Partition แบบ `huge_app.csv` ใน `platformio.ini`

---
**Happy Coding with ESP32!** 🚀
