#pragma once
#include "stdafx.h"
#include <SDK/coreDarkMode.h>   // เพิ่ม: สำหรับ fb2k::CCoreDarkModeHooks

// ========== Instance ==========
class history_preferences_instance : public preferences_page_instance {
public:
    history_preferences_instance(HWND p_parent, preferences_page_callback::ptr callback);
    ~history_preferences_instance();

    t_uint32 get_state() override;
    fb2k::hwnd_t get_wnd() override { return m_hwnd; }
    void apply() override;
    void reset() override;

private:
    HWND m_hwnd = NULL;
    HWND m_font_label = NULL;
    HWND m_font_edit = NULL;
    HWND m_font_hint_label = NULL;
    HWND m_bg_auto_checkbox = NULL;
    HWND m_bg_choose_button = NULL;
    HWND m_bg_preview_label = NULL;
    HWND m_button_height_label = NULL;
    HWND m_button_height_edit = NULL;

    preferences_page_callback::ptr m_callback;

    bool m_bg_auto = true;
    COLORREF m_bg_color = RGB(255, 255, 255);
    bool m_dirty = false;

    fb2k::CCoreDarkModeHooks m_darkMode;   // เพิ่ม: ตัว hook dark mode (createAuto() เรียกใน ctor อัตโนมัติ)

    void create_controls(HWND p_parent);
    void load_from_config();
    void update_bg_controls_enabled();
    void update_bg_preview_text();
    void mark_dirty();
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

// ========== Entry point ==========
class history_preferences_page : public preferences_page_v3 {
public:
    const char* get_name() override;
    GUID get_guid() override;
    GUID get_parent_guid() override;
    preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) override;
};