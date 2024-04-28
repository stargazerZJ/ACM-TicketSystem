//
// Created by zj on 4/27/2024.
//

#include "buffer_pool_manager.h"
#include "b_plus_tree_frame.h"
#include "config.h"
#include <iostream>

void bpt_frame_test() {
  using internal_frame = storage::BPlusTreeInternalFrame<storage::hash_t, storage::page_id_t, 1>;
  using leaf_frame = storage::BPlusTreeLeafFrame<storage::hash_t, int, 1>;
  using leaf_frame2 = storage::BPlusTreeLeafFrame<char, char, 1>;
  std::cout << "Size of a frame with 1 page: " << storage::Frame<1>::kFrameSize << std::endl;
  std::cout << "Size of BPlusTreeInternalFrame<hash_t, page_id_t, 1>: " << sizeof(internal_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<hash_t, int, 1>: " << sizeof(leaf_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<char, char, 1>: " << sizeof(leaf_frame2) << std::endl;
}

int main() {
  bool reset = true;
  storage::BufferPoolManager<1> bpm("test", reset);
  bpt_frame_test();
  return 0;
}