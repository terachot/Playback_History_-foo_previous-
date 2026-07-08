#include "stdafx.h"
#include "history_manager.h"
#include "history_panel.h"

history_manager& history_manager::instance() {
    static history_manager g_instance;
    return g_instance;
}

void history_manager::on_track_changed(const metadb_handle_ptr& p_track) {
    if (p_track.is_empty()) return;

    // กันไม่ให้ push ซ้ำถ้าเป็นเพลงเดียวกับตัวล่าสุดในประวัติ
    if (!m_history.empty() && m_history.back() == p_track) return;

    m_history.push_back(p_track);
    while (m_history.size() > MAX_HISTORY) {
        m_history.pop_front();
    }

    history_ui_element_instance::refresh_all();
}

void history_manager::play_from_history(size_t index) {
    if (index >= m_history.size()) return;
    play_handle(m_history[index]);
}

void history_manager::go_back() {
    // ต้องมีอย่างน้อย 2 รายการถึงจะย้อนได้ (มีเพลงก่อนหน้าให้ย้อนไป)
    if (m_history.size() < 2) return;

    // ลบเพลงปัจจุบัน (ตัวสุดท้าย) ออกจากประวัติ แล้วเล่นเพลงที่กลายเป็นตัวสุดท้ายใหม่
    m_history.pop_back();
    play_handle(m_history.back());

    history_ui_element_instance::refresh_all();
}

void history_manager::play_handle(const metadb_handle_ptr& p_track) {
    static_api_ptr_t<playlist_manager> pm;

    const t_size playlist_count = pm->get_playlist_count();
    t_size found_playlist = pfc_infinite;
    t_size found_idx = pfc_infinite;

    // หาเพลงนี้ใน playlist ทุกอันที่เปิดอยู่ (ไม่ใช่แค่ playlist ที่ active)
    for (t_size p = 0; p < playlist_count && found_playlist == pfc_infinite; p++) {
        metadb_handle_list items;
        pm->playlist_get_all_items(p, items);
        for (t_size i = 0; i < items.get_count(); i++) {
            if (items[i] == p_track) {
                found_playlist = p;
                found_idx = i;
                break;
            }
        }
    }

    if (found_playlist == pfc_infinite) {
        // เพลงนี้ไม่อยู่ใน playlist ไหนเลยแล้ว - ไม่เพิ่มเข้า playlist ใดๆ ตามที่กำหนด
        return;
    }

    // สลับไป playlist ที่เพลงนี้อยู่จริง (เผื่อไม่ตรงกับ playlist ที่ active อยู่ตอนนี้)
    if (pm->get_active_playlist() != found_playlist) {
        pm->set_active_playlist(found_playlist);
    }

    pm->playlist_execute_default_action(found_playlist, found_idx);
}

// ========== Hook เข้ากับ playback event ของ foobar2000 ==========
namespace {
    class history_playback_callback : public play_callback_static {
    public:
        unsigned get_flags() override {
            return flag_on_playback_new_track;
        }

        void on_playback_starting(play_control::t_track_command, bool) override {}
        void on_playback_new_track(metadb_handle_ptr p_track) override {
            history_manager::instance().on_track_changed(p_track);
        }
        void on_playback_stop(play_control::t_stop_reason) override {}
        void on_playback_seek(double) override {}
        void on_playback_pause(bool) override {}
        void on_playback_edited(metadb_handle_ptr) override {}
        void on_playback_dynamic_info(const file_info&) override {}
        void on_playback_dynamic_info_track(const file_info&) override {}
        void on_playback_time(double) override {}
        void on_volume_change(float) override {}
    };

    static service_factory_single_t<history_playback_callback> g_history_playback_callback;
}