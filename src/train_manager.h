//
// Created by zj on 5/13/2024.
//

#pragma once
#include <buffer_pool_manager.h>

namespace storage {
class VarLengthStore;
}

namespace business {

class TrainManager {
  public:
  TrainManager(storage::BufferPoolManager<storage::VLS_PAGES_PER_FRAME> *bpm, storage::VarLengthStore *vls, bool reset) : vls_(vls) {}
  private:
    storage::VarLengthStore *vls_;
};

} // namespace business