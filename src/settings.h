#pragma once
#include "stdafx.h"

// การตั้งค่าที่ปรับได้จาก Preferences > Tools > Playback History
// เก็บถาวรข้ามการเปิด-ปิดโปรแกรมด้วย cfg_int ของ SDK

// ขนาด font (px) ที่ผู้ใช้กำหนดเอง - ค่า 0 = ใช้ font ของ foobar2000 เอง (auto, ตาม Preferences > Display > Fonts)
extern cfg_int g_cfg_font_size;

// สีพื้นหลัง panel ที่ผู้ใช้กำหนดเอง (เก็บเป็นค่า COLORREF) - ค่า -1 = ใช้สีตามธีมของ foobar2000 (auto, รองรับ dark mode)
extern cfg_int g_cfg_bg_color;

// ความสูงของปุ่ม Back (px)
extern cfg_int g_cfg_button_height;

// ค่า default สำหรับปุ่ม Reset ในหน้า Preferences
namespace history_settings_defaults {
    constexpr int64_t font_size = 0;       // 0 = auto
    constexpr int64_t bg_color = -1;       // -1 = auto
    constexpr int64_t button_height = 60;
}

// ขีดจำกัดค่าที่ยอมรับได้ (ป้องกันผู้ใช้กรอกค่าที่ทำให้ UI พังหรือมองไม่เห็น)
namespace history_settings_limits {
    constexpr int64_t font_size_min = 8;
    constexpr int64_t font_size_max = 72;
    constexpr int64_t button_height_min = 20;
    constexpr int64_t button_height_max = 100;
}