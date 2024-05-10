//
// Created by zj on 5/10/2024.
//

#include "lru_k_replacer.h"

namespace storage {
LRUKReplacer::LRUKReplacer(size_t num_frames) {
  evict_hint_.resize(num_frames, evitable_frames_.end());
}
auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  if (evitable_frames_.empty()) {
    return false;
  }
  auto it = evitable_frames_.begin();
  *frame_id = it->second;
  evict_hint_[it->second] = evitable_frames_.end();
  evitable_frames_.erase(it);
  return true;
}
auto LRUKReplacer::RecordAccess(page_id_t page_id) -> void {
  auto &node = access_history_[page_id];
  node.RecordAccess();
}
void LRUKReplacer::SetEvictable(frame_id_t frame_id, page_id_t page_id) {
  if (evict_hint_[frame_id] != evitable_frames_.end()) {
    evitable_frames_.erase(evict_hint_[frame_id]);
  }
  evict_hint_[frame_id] = evitable_frames_.emplace(GetKDistance(page_id), frame_id).first;
}
void LRUKReplacer::SetNonevictable(frame_id_t frame_id) {
  if (evict_hint_[frame_id] != evitable_frames_.end()) {
    evitable_frames_.erase(evict_hint_[frame_id]);
    evict_hint_[frame_id] = evitable_frames_.end();
  }
}
void LRUKReplacer::Remove(frame_id_t frame_id, page_id_t page_id) {
  if (evict_hint_[frame_id] != evitable_frames_.end()) {
    evitable_frames_.erase(evict_hint_[frame_id]);
    evict_hint_[frame_id] = evitable_frames_.end();
  }
  access_history_.erase(page_id);
}
auto LRUKReplacer::Size() const -> size_t {
  return evitable_frames_.size();
}
auto LRUKReplacer::GetKDistance(page_id_t page_id) -> time_distance_t {
  auto &node = access_history_[page_id];
  return node.GetKDistance();
}
void LRUKReplacer::LRUKNode::RecordAccess() {
  ++timestamp_;
  queue[tail] = timestamp_;
  tail = (tail + 1) % replacer_k;
}
auto LRUKReplacer::LRUKNode::GetKDistance() {
  return time_distance_t(queue[tail], queue[(tail + replacer_k - 1) % replacer_k]);
}
} // namespace storage
