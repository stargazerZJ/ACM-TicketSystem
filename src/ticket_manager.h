//
// Created by zj on 5/13/2024.
//

#pragma once
#include <buffer_pool_manager.h>
#include <b_plus_tree.h>
#include <variable_length_store.h>

namespace business {
enum class TicketStatus : char {
  SUCCESS = 'S',
  PENDING = 'P',
  REFUNDED = 'R'
};

struct TicketInfo {
  TicketStatus status; // 'S' for success, 'P' for pending, 'R' for refunded
  date_t date; // the day when the train depart from the TRAIN's departing station
  int8_t from;
  int8_t to;
  storage::record_id_t train_id;
  int seat_count;
};

struct TicketSimpleInfo {
  int8_t from;
  int8_t to;
  order_no_t order_no; // this ticket is the `order_no`-th ticket of the user
  int seat_count;
  storage::record_id_t user_id;
};

class TicketManager {
  public:
    TicketManager(storage::BufferPoolManager<storage::VLS_PAGES_PER_FRAME> *bpm,
                  storage::VarLengthStore *vls,
                  bool reset) : ticket_index_(bpm, bpm->AllocateInfo(), reset),
                                pending_queue_(bpm, bpm->AllocateInfo(), reset) {
    }

  protected:
    storage::BPlusTree<
      storage::PackedPair<storage::record_id_t, order_no_t>,
      TicketInfo> ticket_index_; // <user_id, -order_no> -> ticket_info
    storage::BPlusTree<
      storage::PackedPair<
        storage::PackedPair<storage::record_id_t, date_t>,
        int>,
      TicketSimpleInfo> pending_queue_; // < <train_id, depart_date>, timestamp> -> ticket_simple_info
};
} // namespace business
