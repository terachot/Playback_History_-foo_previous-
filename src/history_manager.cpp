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

    t_size playlist = pm->get_active_playlist();
    if (playlist == pfc_infinite) {
        playlist = pm->create_playlist("Default", pfc_infinite, pfc_infinite);
        pm->set_active_playlist(playlist);
    }

    // เช็คก่อนว่าเพลงนี้อยู่ใน playlist ปัจจุบันอยู่แล้วหรือยัง
    metadb_handle_list playlist_items;
    pm->playlist_get_all_items(playlist, playlist_items);

    t_size found_idx = pfc_infinite;
    for (t_size i = 0; i < playlist_items.get_count(); i++) {
        if (playlist_items[i] == p_track) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == pfc_infinite) {
        // ไม่มีอยู่ใน playlist นี้ - เพิ่มเข้าไปท้ายสุด
        found_idx = pm->playlist_get_item_count(playlist);
        metadb_handle_list list;
        list.add_item(p_track);
        pm->playlist_insert_items(playlist, found_idx, list, pfc::bit_array_false());
    }

    // เล่นจากตำแหน่งที่เจอ (หรือตำแหน่งที่เพิ่งเพิ่มเข้าไป)
    pm->playlist_execute_default_action(playlist, found_idx);
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