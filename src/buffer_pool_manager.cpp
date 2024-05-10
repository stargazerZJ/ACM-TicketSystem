//
// Created by zj on 4/29/2024.
//

#include "buffer_pool_manager.h"

namespace storage {

// auto BufferPoolManager<1>::NewFrameGuarded(page_id_t *page_id) -> BasicFrameGuard {
//   auto frame = new Frame<1>();
//   if (page_id == nullptr) {
//     page_id = &frame->page_id_;
//   }
//   *page_id = disk_.newPage(reinterpret_cast<const int *>(frame->GetData()));
//   frame->page_id_ = *page_id;
//   return {this, frame};
// }
// auto BufferPoolManager<1>::FetchFrameBasic(page_id_t page_id) -> BasicFrameGuard {
//   auto frame = new Frame<1>();
//   disk_.getPage(page_id, reinterpret_cast<int *>(frame->GetData()));
//   frame->page_id_ = page_id;
//   return {this, frame};
// }

}