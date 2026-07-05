#pragma once
#include "stdafx.h"

// GUID ของ UI Element นี้ - ต้องตรงกันทั้ง history_ui_element และ history_ui_element_instance
extern const GUID guid_history_ui_element;

// ========== Instance ==========
// panel ตัวจริงที่ถูกสร้างขึ้นเมื่อ host (Default UI) เรียก instantiate()
// หน้าต่างของ instance นี้เป็น child window ของ HWND ที่ host ส่งมาให้เสมอ
// (ถ้า host เปิดเป็น popup ตาม get_popup_specs() ของ history_ui_element ก็จะดูเหมือนหน้าต่างลอยแยกต่างหาก)
class history_ui_element_instance : public ui_element_instance {
public:
    history_ui_element_instance(fb2k::hwnd_t p_parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr callback);
    ~history_ui_element_instance();

    // ===== ui_element_instance interface =====
    fb2k::hwnd_t get_wnd() override { return m_hwnd; }
    void set_configuration(ui_element_config::ptr data) override { (void)data; }
    ui_element_config::ptr get_configuration() override;
    GUID get_guid() override;
    GUID get_subclass() override;
    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) override;

    // เรียกทุกครั้งที่ history เปลี่ยน เพื่ออัปเดตรายการที่แสดงในทุก instance ที่เปิดอยู่ (อาจมีมากกว่า 1 พร้อมกัน)
    static void refresh_all();

private:
    HWND m_hwnd = NULL;
    HWND m_listbox = NULL;
    HWND m_back_button = NULL;
    t_ui_font m_font = NULL; // ไม่ได้เป็นเจ้าของ - มาจาก host ผ่าน query_font_ex()
    ui_element_instance_callback_ptr m_callback;

    COLORREF m_color_bg = RGB(255, 255, 255);
    COLORREF m_color_text = RGB(0, 0, 0);
    HBRUSH m_brush_bg = NULL; // เราเป็นเจ้าของ ต้อง DeleteObject เอง

    void create_controls(HWND p_parent);
    void layout_controls();
    void populate_list();
    void apply_font();
    void apply_colors();

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

// ========== Entry point ==========
// บอก Default UI ว่ามี UI Element แบบนี้อยู่ (โผล่ใน Add New UI Element / auto-gen เมนูเปิด popup)
class history_ui_element : public ui_element_v2 {
public:
    GUID get_guid() override;
    GUID get_subclass() override;
    void get_name(pfc::string_base& p_out) override;
    ui_element_instance_ptr instantiate(fb2k::hwnd_t p_parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr p_callback) override;
    ui_element_config::ptr get_default_configuration() override;
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) override { (void)cfg; return NULL; }

    // KFlagHavePopupCommand: ให้ SDK auto-gen เมนูคำสั่งเปิด panel นี้เป็น popup window ให้เอง
    t_uint32 get_flags() override;
    bool bump() override { return false; }
    bool get_popup_specs(ui_size& defSize, pfc::string_base& title) override;
};