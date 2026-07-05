#include "stdafx.h"
#include "settings.h"

// GUID เฉพาะของแต่ละค่า - ห้ามเปลี่ยนหลัง release ไปแล้ว ไม่งั้นค่าที่ผู้ใช้ตั้งไว้จะรีเซ็ตกลับเป็น default
namespace {
    const GUID guid_cfg_font_size =
    { 0xb1c2d3e4, 0xf5a6, 0x4b7c, { 0x8d, 0x9e, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x01 } };
    const GUID guid_cfg_bg_color =
    { 0xb1c2d3e4, 0xf5a6, 0x4b7c, { 0x8d, 0x9e, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x02 } };
    const GUID guid_cfg_button_height =
    { 0xb1c2d3e4, 0xf5a6, 0x4b7c, { 0x8d, 0x9e, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x03 } };
}

cfg_int g_cfg_font_size(guid_cfg_font_size, history_settings_defaults::font_size);
cfg_int g_cfg_bg_color(guid_cfg_bg_color, history_settings_defaults::bg_color);
cfg_int g_cfg_button_height(guid_cfg_button_height, history_settings_defaults::button_height);