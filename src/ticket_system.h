//
// Created by zj on 5/13/2024.
//

#pragma once

#include <buffer_pool_manager.h>
#include <variable_length_store.h>

#include <string>

#include "ticket_manager.h"
#include "user_manager.h"
#include "train_manager.h"

namespace business {
class TicketSystem : public UserManager, public TicketManager, public TrainManager {
  public:
    explicit TicketSystem(const std::string &db_file_name, bool reset = false) : db_file_name(db_file_name),
      bpm_(db_file_name, reset), vls_(&bpm_, bpm_.AllocateInfo(), reset), UserManager(&bpm_, &vls_, reset), TicketManager(&bpm_, &vls_, reset),
      TrainManager(&bpm_, &vls_, reset) {
    }

  private:
    const std::string db_file_name;
    storage::BufferPoolManager<1> bpm_;
    storage::VarLengthStore vls_;
};
} // namespace business
