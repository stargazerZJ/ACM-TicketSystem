//
// Created by zj on 5/13/2024.
//

#pragma once

#include <buffer_pool_manager.h>
#include <hash.h>
#include <variable_length_store.h>

#include <string>

#include "ticket_manager.h"
#include "user_manager.h"
#include "train_manager.h"

namespace business {
class TicketSystemBase {
  protected:
    TicketSystemBase(const std::string &db_file_name, bool reset)
      : db_file_name_(db_file_name), bpm_(db_file_name, reset), vls_(&bpm_, bpm_.AllocateInfo(), reset) {
    }

    const std::string db_file_name_;
    storage::BufferPoolManager<1> bpm_;
    storage::VarLengthStore vls_;
};
class TicketSystem : public TicketSystemBase, public UserManager, public TicketManager, public TrainManager {
  public:
    explicit TicketSystem(const std::string &db_file_name, bool reset = false) : TicketSystemBase(db_file_name, reset),
      UserManager(&bpm_, &(TicketSystemBase::vls_), reset),
      TicketManager(&bpm_, &(TicketSystemBase::vls_), reset),
      TrainManager(&bpm_, &(TicketSystemBase::vls_), reset) {
    }

    void BuyTicket(int timestamp,
                   std::string_view username,
                   std::string_view train_name,
                   date_t date,
                   std::string_view from_str,
                   std::string_view to_str,
                   int seat_count,
                   bool agree_to_wait);

    void QueryOrder(std::string_view username);

    void RefundTicket(std::string_view username, order_no_t order_no);

  private:
    storage::VarLengthStore *vls() { return &(TicketSystemBase::vls_); }
};
} // namespace business
