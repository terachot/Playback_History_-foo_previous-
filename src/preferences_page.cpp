#include "stdafx.h"
#include "preferences_page.h"
#include "settings.h"
#include "history_panel.h"
#include <string>
#include <commdlg.h>

namespace {
    const wchar_t* WND_CLASS_NAME = L"MyFb2kHistoryPreferencesClass";

    const int ID_FONT_EDIT = 2001;
    const int ID_BG_AUTO_CHECKBOX = 2002;
    const int ID_BG_CHOOSE_BUTTON = 2003;
    const int ID_BUTTON_HEIGHT_EDIT = 2004;

    // GUID ของหน้า Preferences นี้ - สร้างใหม่ได้ด้วย Visual Studio: Tools > Create GUID
    const GUID guid_history_preferences_page =
    { 0xc2d3e4f5, 0x2233, 0x4c6d, { 0xad, 0x4e, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd } };
}

// ========== history_preferences_instance ==========

history_preferences_instance::history_preferences_instance(HWND p_parent, preferences_page_callback::ptr callback)
    : m_callback(callback)
{
    create_controls(p_parent);
    load_from_config();
}

history_preferences_instance::~history_preferences_instance() {
    if (m_hwnd && IsWindow(m_hwnd)) {
        DestroyWindow(m_hwnd);
    }
}

t_uint32 history_preferences_instance::get_state() {
    t_uint32 state = preferences_state::dark_mode_supported;   // เพิ่ม: ประกาศว่ารองรับ dark mode เสมอ
    if (m_dirty) state |= preferences_state::changed;
    return state;
}

void history_preferences_instance::create_controls(HWND p_parent) {
    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = wnd_proc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = WND_CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);
        class_registered = true;
    }

    m_hwnd = CreateWindowExW(
        0, WND_CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        p_parent, NULL, GetModuleHandle(NULL), this
    );

    HINSTANCE hInst = GetModuleHandle(NULL);
    int y = 16;
    const int label_h = 20;
    const int row_gap = 32;
    const int left = 16;

    m_font_label = CreateWindowW(L"STATIC", L"Font size (px):",
        WS_CHILD | WS_VISIBLE, left, y, 150, label_h, m_hwnd, NULL, hInst, NULL);
    m_font_edit = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        left + 155, y - 2, 60, 22, m_hwnd, (HMENU)(INT_PTR)ID_FONT_EDIT, hInst, NULL);
    m_font_hint_label = CreateWindowW(L"STATIC", L"(0 = use foobar2000's own list font)",
        WS_CHILD | WS_VISIBLE, left + 225, y, 260, label_h, m_hwnd, NULL, hInst, NULL);
    y += row_gap;

    m_bg_auto_checkbox = CreateWindowW(L"BUTTON", L"Use foobar2000 theme background color",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        left, y, 300, label_h, m_hwnd, (HMENU)(INT_PTR)ID_BG_AUTO_CHECKBOX, hInst, NULL);
    y += row_gap;

    m_bg_choose_button = CreateWindowW(L"BUTTON", L"Choose Color...",
        WS_CHILD | WS_VISIBLE,
        left, y, 130, 24, m_hwnd, (HMENU)(INT_PTR)ID_BG_CHOOSE_BUTTON, hInst, NULL);
    m_bg_preview_label = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER,
        left + 140, y, 100, 24, m_hwnd, NULL, hInst, NULL);
    y += row_gap;

    m_button_height_label = CreateWindowW(L"STATIC", L"Back button height (px):",
        WS_CHILD | WS_VISIBLE, left, y, 150, label_h, m_hwnd, NULL, hInst, NULL);
    m_button_height_edit = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        left + 155, y - 2, 60, 22, m_hwnd, (HMENU)(INT_PTR)ID_BUTTON_HEIGHT_EDIT, hInst, NULL);
    y += row_gap;

    HFONT ui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND children[] = { m_font_label, m_font_edit, m_font_hint_label, m_bg_auto_checkbox,
                         m_bg_choose_button, m_bg_preview_label, m_button_height_label, m_button_height_edit };
    for (HWND child : children) {
        if (child) SendMessage(child, WM_SETFONT, (WPARAM)ui_font, TRUE);
    }
    // เพิ่ม: เปิด dark mode ให้ window ตัวเองและ standard controls ทั้งหมด
    // ต้องเรียกหลังจาก child controls ถูกสร้างครบแล้วเท่านั้น (AddControls ต้อง enum children)
    m_darkMode.AddDialogWithControls(m_hwnd);
}

void history_preferences_instance::load_from_config() {
    int64_t font_size = g_cfg_font_size.get();
    SetWindowTextW(m_font_edit, std::to_wstring(font_size).c_str());

    int64_t bg = g_cfg_bg_color.get();
    m_bg_auto = (bg < 0);
    m_bg_color = m_bg_auto ? RGB(255, 255, 255) : (COLORREF)bg;
    SendMessage(m_bg_auto_checkbox, BM_SETCHECK, m_bg_auto ? BST_CHECKED : BST_UNCHECKED, 0);

    int64_t button_height = g_cfg_button_height.get();
    SetWindowTextW(m_button_height_edit, std::to_wstring(button_height).c_str());

    update_bg_controls_enabled();
    update_bg_preview_text();

    m_dirty = false;
}

void history_preferences_instance::update_bg_controls_enabled() {
    EnableWindow(m_bg_choose_button, !m_bg_auto);
}

void history_preferences_instance::update_bg_preview_text() {
    if (m_bg_auto) {
        SetWindowTextW(m_bg_preview_label, L"(theme)");
    }
    else {
        wchar_t buf[16];
        wsprintfW(buf, L"#%02X%02X%02X", GetRValue(m_bg_color), GetGValue(m_bg_color), GetBValue(m_bg_color));
        SetWindowTextW(m_bg_preview_label, buf);
    }
}

void history_preferences_instance::mark_dirty() {
    if (!m_dirty) {
        m_dirty = true;
        if (m_callback.is_valid()) m_callback->on_state_changed();
    }
}

void history_preferences_instance::apply() {
    int64_t font_size = GetDlgItemInt(m_hwnd, ID_FONT_EDIT, NULL, FALSE);
    if (font_size != 0) {
        if (font_size < history_settings_limits::font_size_min) font_size = history_settings_limits::font_size_min;
        if (font_size > history_settings_limits::font_size_max) font_size = history_settings_limits::font_size_max;
    }
    g_cfg_font_size = font_size;

    g_cfg_bg_color = m_bg_auto ? -1 : (int64_t)m_bg_color;

    int64_t button_height = GetDlgItemInt(m_hwnd, ID_BUTTON_HEIGHT_EDIT, NULL, FALSE);
    if (button_height < history_settings_limits::button_height_min) button_height = history_settings_limits::button_height_min;
    if (button_height > history_settings_limits::button_height_max) button_height = history_settings_limits::button_height_max;
    g_cfg_button_height = button_height;

    // อัปเดต panel ที่เปิดอยู่ทั้งหมดให้เห็นผลทันที ไม่ต้องปิดเปิดใหม่
    history_ui_element_instance::apply_settings_all();

    m_dirty = false;
    if (m_callback.is_valid()) m_callback->on_state_changed();
}

void history_preferences_instance::reset() {
    SetWindowTextW(m_font_edit, std::to_wstring(history_settings_defaults::font_size).c_str());

    m_bg_auto = (history_settings_defaults::bg_color < 0);
    m_bg_color = RGB(255, 255, 255);
    SendMessage(m_bg_auto_checkbox, BM_SETCHECK, m_bg_auto ? BST_CHECKED : BST_UNCHECKED, 0);

    SetWindowTextW(m_button_height_edit, std::to_wstring(history_settings_defaults::button_height).c_str());

    update_bg_controls_enabled();
    update_bg_preview_text();

    mark_dirty();
}

LRESULT CALLBACK history_preferences_instance::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lparam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    history_preferences_instance* self =
        (history_preferences_instance*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!self) return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg) {
    case WM_ERASEBKGND: {
        if ((bool)self->m_darkMode) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            HDC hdc = (HDC)wparam;
            static HBRUSH s_dark_bg_brush = CreateSolidBrush(RGB(32, 32, 32));
            FillRect(hdc, &rc, s_dark_bg_brush);
            return 1;  // บอกว่า erase เสร็จแล้ว ไม่ต้องให้ DefWindowProc ใช้ class brush เดิมตามปกติ
        }
        break;  // light mode: ปล่อยให้ DefWindowProc ใช้ class brush เดิมตามปกติ
    }
    case WM_COMMAND: {
        int id = LOWORD(wparam);
        int notify = HIWORD(wparam);

        if (id == ID_FONT_EDIT && notify == EN_CHANGE) {
            self->mark_dirty();
            return 0;
        }
        if (id == ID_BUTTON_HEIGHT_EDIT && notify == EN_CHANGE) {
            self->mark_dirty();
            return 0;
        }
        if (id == ID_BG_AUTO_CHECKBOX && notify == BN_CLICKED) {
            bool checked = (SendMessage(self->m_bg_auto_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
            self->m_bg_auto = checked;
            self->update_bg_controls_enabled();
            self->update_bg_preview_text();
            self->mark_dirty();
            return 0;
        }
        if (id == ID_BG_CHOOSE_BUTTON && notify == BN_CLICKED) {
            static COLORREF custom_colors[16] = { 0 };
            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.rgbResult = self->m_bg_color;
            cc.lpCustColors = custom_colors;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                self->m_bg_color = cc.rgbResult;
                self->update_bg_preview_text();
                self->mark_dirty();
            }
            return 0;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

// ========== history_preferences_page ==========

const char* history_preferences_page::get_name() {
    return "Playback History";
}

GUID history_preferences_page::get_guid() {
    return guid_history_preferences_page;
}

GUID history_preferences_page::get_parent_guid() {
    return preferences_page::guid_tools;
}

preferences_page_instance::ptr history_preferences_page::instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) {
    return new service_impl_t<history_preferences_instance>((HWND)parent, callback);
}

// ========== Register Service ==========
static preferences_page_factory_t<history_preferences_page> g_history_preferences_page_factory;