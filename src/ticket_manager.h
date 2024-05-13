//
// Created by zj on 5/13/2024.
//

#pragma once
#include <buffer_pool_manager.h>
#include <variable_length_store.h>

namespace business {

class TicketManager {
  public:
  TicketManager(storage::BufferPoolManager<storage::VLS_PAGES_PER_FRAME> *bpm, storage::VarLengthStore *vls, bool reset) : vls_(vls) {}
  private:
    storage::VarLengthStore *vls_;
};

} // namespace business