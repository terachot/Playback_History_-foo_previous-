#pragma once
#include "stdafx.h"
#include <deque>

// เก็บประวัติเพลงที่เล่นผ่านมา และช่วยสั่งเล่นเพลงจากประวัติ
// รายการเรียงจากเก่าสุด -> ล่าสุด (index สุดท้าย = เพลงที่กำลังเล่น/เพิ่งเล่นล่าสุด)
class history_manager {
public:
    static history_manager& instance();

    // เรียกทุกครั้งที่เพลงเปลี่ยน (ถูกเรียกจาก play_callback)
    void on_track_changed(const metadb_handle_ptr& p_track);

    const std::deque<metadb_handle_ptr>& get_history() const { return m_history; }

    // เล่นเพลงจาก index ใน get_history() (0 = เก่าสุด)
    void play_from_history(size_t index);

    // ย้อนกลับไปเล่นเพลงก่อนหน้าเพลงปัจจุบัน 1 เพลง
    void go_back();

private:
    static const size_t MAX_HISTORY = 20;
    std::deque<metadb_handle_ptr> m_history;

    void play_handle(const metadb_handle_ptr& p_track);
};