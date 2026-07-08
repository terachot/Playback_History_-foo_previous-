#include "stdafx.h"
#include "history_panel.h"
#include "history_manager.h"
#include "settings.h"
#include <vector>
#include <algorithm>

// GUID เฉพาะของ UI Element นี้ - สร้างใหม่ได้ด้วย Visual Studio: Tools > Create GUID
const GUID guid_history_ui_element =
{ 0xa1b2c3d4, 0x1122, 0x4a5b, { 0x9c, 0x3d, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc } };

namespace {
    const wchar_t* WND_CLASS_NAME = L"MyFb2kHistoryPanelElementClass";
    const int ID_LISTBOX = 1001;
    const int ID_BACK_BUTTON = 1002;

    // รายการ instance ที่เปิดอยู่ทั้งหมด (อาจมีมากกว่า 1 พร้อมกัน เช่น popup + docked)
    std::vector<history_ui_element_instance*> g_instances;
}

// ========== history_ui_element_instance ==========

history_ui_element_instance::history_ui_element_instance(fb2k::hwnd_t p_parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr callback)
    : m_callback(callback)
{
    (void)cfg; // ไม่มีข้อมูลให้ persist ในตอนนี้
    create_controls((HWND)p_parent);
    g_instances.push_back(this);
}

history_ui_element_instance::~history_ui_element_instance() {
    g_instances.erase(std::remove(g_instances.begin(), g_instances.end(), this), g_instances.end());

    if (m_brush_bg) {
        DeleteObject(m_brush_bg);
        m_brush_bg = NULL;
    }
    if (m_custom_font) {
        DeleteObject(m_custom_font);
        m_custom_font = NULL;
    }
    if (m_hwnd && IsWindow(m_hwnd)) {
        DestroyWindow(m_hwnd);
    }
}

ui_element_config::ptr history_ui_element_instance::get_configuration() {
    return ui_element_config::g_create_empty(get_guid());
}

GUID history_ui_element_instance::get_guid() {
    return guid_history_ui_element;
}

GUID history_ui_element_instance::get_subclass() {
    return ui_element_subclass_utility;
}

void history_ui_element_instance::create_controls(HWND p_parent) {
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

    // สำคัญ: WS_CHILD เสมอ เพราะ host (Default UI) เป็นคนสร้าง/จัดการหน้าต่างนอกสุดให้
    // ถ้า host เปิดผ่าน popup command จะดูเหมือนหน้าต่างลอยแยกต่างหาก แต่จริงๆ แล้วเราเป็นแค่ child อยู่ดี
    m_hwnd = CreateWindowExW(
        0,
        WND_CLASS_NAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        p_parent, NULL, GetModuleHandle(NULL), this
    );

    m_listbox = CreateWindowW(L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        0, 0, 0, 0, m_hwnd, (HMENU)(INT_PTR)ID_LISTBOX, GetModuleHandle(NULL), NULL);

    m_back_button = CreateWindowW(L"BUTTON", L"\u25C0 Back (\u0e22\u0e49\u0e2d\u0e19\u0e01\u0e25\u0e31\u0e1a)",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, (HMENU)(INT_PTR)ID_BACK_BUTTON, GetModuleHandle(NULL), NULL);

    m_darkMode.AddCtrlAuto(m_back_button);   // เพิ่ม

    apply_font();
    apply_colors();
    layout_controls();
    populate_list();
}

void history_ui_element_instance::apply_font() {
    int64_t custom_size = g_cfg_font_size.get();

    if (m_custom_font) {
        DeleteObject(m_custom_font);
        m_custom_font = NULL;
    }

    if (custom_size > 0) {
        // ใช้ face name เดียวกับ font ของ host เพื่อให้ยังดูเข้ากับสไตล์เดิม แค่ override ขนาด
        LOGFONTW lf = {};
        HFONT host_font = m_callback.is_valid() ? (HFONT)m_callback->query_font_ex(ui_font_lists) : NULL;
        if (host_font && GetObjectW(host_font, sizeof(lf), &lf)) {
            // มีค่าจาก host แล้ว แค่ override lfHeight ด้านล่าง
        }
        else {
            wcscpy_s(lf.lfFaceName, L"Segoe UI");
            lf.lfWeight = FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfQuality = CLEARTYPE_QUALITY;
        }
        lf.lfHeight = -(int)custom_size;
        m_custom_font = CreateFontIndirectW(&lf);
        m_font = m_custom_font;
    }
    else {
        // 0 = auto: ใช้ font ของ foobar2000 เอง (Preferences > Display > Fonts)
        m_font = m_callback.is_valid() ? m_callback->query_font_ex(ui_font_lists) : NULL;
    }

    if (m_font) {
        if (m_listbox) SendMessage(m_listbox, WM_SETFONT, (WPARAM)m_font, TRUE);
        if (m_back_button) SendMessage(m_back_button, WM_SETFONT, (WPARAM)m_font, TRUE);
    }
}

void history_ui_element_instance::apply_colors() {
    int64_t custom_bg = g_cfg_bg_color.get();

    if (custom_bg >= 0) {
        m_color_bg = (COLORREF)custom_bg;
        // สีตัวหนังสือยังตามธีมเสมอ เพื่อให้อ่านออกไม่ว่าจะตั้งพื้นหลังเป็นสีอะไร
        if (m_callback.is_valid()) m_color_text = (COLORREF)m_callback->query_std_color(ui_color_text);
    }
    else if (m_callback.is_valid()) {
        m_color_bg = (COLORREF)m_callback->query_std_color(ui_color_background);
        m_color_text = (COLORREF)m_callback->query_std_color(ui_color_text);
    }

    if (m_brush_bg) {
        DeleteObject(m_brush_bg);
    }
    m_brush_bg = CreateSolidBrush(m_color_bg);

    if (m_hwnd && IsWindow(m_hwnd)) {
        InvalidateRect(m_hwnd, NULL, TRUE);
        if (m_listbox) InvalidateRect(m_listbox, NULL, TRUE);
    }
}

void history_ui_element_instance::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) {
    (void)p_param1; (void)p_param2; (void)p_param2size;
    if (p_what == ui_element_notify_font_changed) {
        apply_font();
    }
    if (p_what == ui_element_notify_colors_changed) {
        apply_colors();
    }
}

void history_ui_element_instance::layout_controls() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int button_area_height = (int)g_cfg_button_height.get(); // ปรับได้จาก Preferences
    int button_padding = 3;       // เว้นขอบบน-ล่างของปุ่ม 3px

    if (m_back_button) {
        int inner_height = button_area_height - (button_padding * 2);
        if (inner_height < 1) inner_height = 1;
        MoveWindow(m_back_button, 0, button_padding, width, inner_height, TRUE);
    }
    if (m_listbox) MoveWindow(m_listbox, 0, button_area_height, width, height - button_area_height, TRUE);
}

void history_ui_element_instance::populate_list() {
    if (!m_listbox) return;
    SendMessage(m_listbox, LB_RESETCONTENT, 0, 0);

    static_api_ptr_t<titleformat_compiler> compiler;
    service_ptr_t<titleformat_object> script;
    compiler->compile_safe_ex(script, "%artist% - %title%");

    const auto& history = history_manager::instance().get_history();
    // แสดงจากล่าสุดไปเก่าสุด (บนสุดของลิสต์ = เพลงปัจจุบัน/ล่าสุด)
    for (size_t i = history.size(); i > 0; i--) {
        pfc::string8 text;
        history[i - 1]->format_title(NULL, text, script, NULL);
        pfc::stringcvt::string_wide_from_utf8 wide(text);
        SendMessageW(m_listbox, LB_ADDSTRING, 0, (LPARAM)wide.get_ptr());
    }
}

void history_ui_element_instance::refresh_all() {
    for (auto* instance : g_instances) {
        instance->populate_list();
    }
}

void history_ui_element_instance::apply_settings_all() {
    for (auto* instance : g_instances) {
        instance->apply_font();
        instance->apply_colors();
        instance->layout_controls();
    }
}

LRESULT CALLBACK history_ui_element_instance::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lparam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    history_ui_element_instance* self =
        (history_ui_element_instance*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (!self) return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg) {
    case WM_SIZE:
        self->layout_controls();
        return 0;
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wparam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, self->m_brush_bg ? self->m_brush_bg : (HBRUSH)(COLOR_BTNFACE + 1));
        return 1;
    }
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wparam;
        SetTextColor(hdc, self->m_color_text);
        SetBkColor(hdc, self->m_color_bg);
        return (LRESULT)(self->m_brush_bg ? self->m_brush_bg : (HBRUSH)(COLOR_BTNFACE + 1));
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == ID_BACK_BUTTON && HIWORD(wparam) == BN_CLICKED) {
            history_manager::instance().go_back();
            return 0;
        }
        if (LOWORD(wparam) == ID_LISTBOX && HIWORD(wparam) == LBN_DBLCLK) {
            int sel = (int)SendMessage(self->m_listbox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                const auto& history = history_manager::instance().get_history();
                // ลิสต์ที่แสดงกลับด้าน (แถวบนสุด = ล่าสุด) ต้องแปลง index กลับ
                size_t history_index = history.size() - 1 - (size_t)sel;
                history_manager::instance().play_from_history(history_index);
            }
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

// ========== history_ui_element ==========

GUID history_ui_element::get_guid() {
    return guid_history_ui_element;
}

GUID history_ui_element::get_subclass() {
    return ui_element_subclass_utility;
}

void history_ui_element::get_name(pfc::string_base& p_out) {
    p_out = "Playback History";
}

ui_element_instance_ptr history_ui_element::instantiate(fb2k::hwnd_t p_parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr p_callback) {
    service_ptr_t<history_ui_element_instance> instance = new service_impl_t<history_ui_element_instance>(p_parent, cfg, p_callback);
    return instance;
}

ui_element_config::ptr history_ui_element::get_default_configuration() {
    return ui_element_config::g_create_empty(get_guid());
}

t_uint32 history_ui_element::get_flags() {
    return KFlagHavePopupCommand;
}

bool history_ui_element::get_popup_specs(ui_size& defSize, pfc::string_base& title) {
    defSize.cx = 300;
    defSize.cy = 400;
    title = "Playback History";
    return true;
}

// ========== Register Service ==========
static service_factory_single_t<history_ui_element> g_history_ui_element_factory;