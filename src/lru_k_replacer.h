//
// Created by zj on 5/10/2024.
//

#pragma once
#include <config.h>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace storage {
class LRUKReplacer {
  public:
    using frame_id_t = int;
    using timestamp_t = uint32_t;
    using time_distance_t = std::pair<timestamp_t, timestamp_t>;
    explicit LRUKReplacer(size_t pool_size);

    /// Will not clear the access history
    auto Evict(frame_id_t *frame_id) -> bool;

    auto RecordAccess(page_id_t page_id) -> void;

    void SetEvictable(frame_id_t frame_id, page_id_t page_id);

    void SetNonevictable(frame_id_t frame_id);

    void Remove(frame_id_t frame_id, page_id_t page_id);

    auto Size() const -> size_t;

  private:
    static constexpr int replacer_k = LRU_REPLACER_K;
    inline static timestamp_t timestamp_ = 0;
    auto GetKDistance(page_id_t page_id) -> time_distance_t;
    class LRUKNode {
      public:
        void RecordAccess();
        auto GetKDistance() -> time_distance_t;

      private:
        timestamp_t queue[replacer_k] = {};
        int tail = 0;
    };
    LRUKNode access_history_[MAX_PAGE_ID]; // the Key is page_id because the history is page-based
    std::map<time_distance_t, frame_id_t> evitable_frames_;
    std::vector<decltype(evitable_frames_)::iterator> evict_hint_;
};
} // namespace storage
