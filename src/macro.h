#include "utils/Remote/Remote.h"
#include "Logger.h"

Remote remote(0x20, 21, 22);

// forward declarations
void m_hardreset();
void m_start(uint8_t mode);
void m_end();
void m_home();
void m_addTimes(uint8_t loop);
void m_disableVoiceCMD();
void m_setMassageSTR();
void m_setBalloonSTR();
void acPower(bool sw);

// ===========================================================
// ##### remote Marcos #####
// ===========================================================

void m_remote_init()
{
    remote.begin();
    log("MACRO: Remote Initialized");
}

void m_hardreset()
{
    // กระบวนการนี้ใช้เวลาประมาณ 50วินาที

    log("MACRO: HARD RESET");
    remote.swOk(); // เอาไว้เทสว่า remote ยัง responsive อยู่ไหม ถ้าไม่มีก็แสดงว่าสายหลวม

    remote.setAcPower(true); // จ่ายไฟ
    remote.wait(2000);       // รอ remote ทำงาน

    remote.swOn(); // power On

    remote.wait(10000);  // รอเก้าอี้กลับสู่ท่านั่ง
    m_disableVoiceCMD(); // ปิดคำสั่งเสียง
    remote.wait(5000);   // รอเก้าอี้กลับสู่ท่านั่ง

    // ทดลองปิดเสียงระหว่างรอเก้่าอีลง ลดเวลา delay
    // click(on);   // power On
    // m_disableVoiceCMD(); // ปิดคำสั่งเสียง
    // remote.wait(15000); // รอเก้าอี้กลับสู่ท่านั่ง

    remote.swOn(); // power Off
    // remote.wait(5000);
    remote.setAcPower(false); // ตัดไฟ

    log("MACRO: HARD RESET >> Success!");
}

void m_start(uint8_t mode)
{
    // กระบวนการนี้ใช้เวลาประมาณ 72วินาที++

    log("MACRO: START");
    remote.setAcPower(true); // จ่ายไฟ
    // remote.wait(2000);
    remote.swOn(); // power On
    // --- ย้ายไป ช่วงก่อน start เพิ่ม UX ที่ดี ---
    // setAcPower(true);
    // remote.wait(1000);

    // click(on);
    // remote.wait(10000);
    // --- x ---

    remote.swDown(mode); // select mode
    remote.swOk();

    remote.wait(15000); // รอเครื่องทำขั้นตอน body-scan 30วินาที
    remote.swOk();      // skip

    // m_setMassageSTR();
    m_setBalloonSTR();

    // Add Machine Massage times
    if (mode > 0)
    {
        m_addTimes(2); // any mode
    }
    else
    {
        m_addTimes(3); // quick mode
    }

    log("MACRO: START >> Success!");
}

void m_end()
{
    // กระบวนการนี้ใช้เวลาประมาณ 31.5วินาที
    log("MACRO: END");
    remote.swOn();
    remote.wait(30000);
    remote.setAcPower(false);
    log("MACRO: END >> Success!");
}

void m_home()
{
    // กระบวนการนี้ใช้เวลาประมาณ 4.5วินาที
    remote.swLeft(3);
    log("Back to HOME..");
}

void m_addTimes(uint8_t loop)
{
    remote.swOk();
    remote.swDown(3);
    remote.swRight();
    remote.swOk(loop);
    m_home();
    //   log("ADD Machine Time: +%dMin", loop * 10);
}

void m_disableVoiceCMD()
{
    // กระบวนการนี้ใช้เวลาประมาณ 15วินาที
    remote.swMenu();
    remote.swDown(4);
    remote.swOk();
    remote.swDown(2);
    remote.swRight();
    remote.swOk();
    log("Disable Voice Command Success!");
}

void m_setMassageSTR()
{
    // กระบวนการนี้ใช้เวลาประมาณ 10.5วินาที
    remote.swOk();
    // remote.swDown();
    remote.swRight();
    remote.swOk();
    m_home();

    log("Set Massage STR Success!");
}

void m_setBalloonSTR()
{
    // กระบวนการนี้ใช้เวลาประมาณ 10.5วินาที
    remote.swOk();
    remote.swDown();
    remote.swRight();
    remote.swDown();
    remote.swOk();
    m_home();

    log("Set Balloon STR Success!");
}

void acPower(bool sw)
{
    // กระบวนการนี้ใช้เวลาประมาณ 5วินาที
    remote.setAcPower(sw);
}