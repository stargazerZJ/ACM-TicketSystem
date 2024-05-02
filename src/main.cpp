//
// Created by zj on 4/27/2024.
//

#include "buffer_pool_manager.h"
#include "b_plus_tree.h"
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

void bpt_test(storage::BufferPoolManager<1> &bpm) {
  /*
   * Input format (stdin):
   * <command> <key> <value>
   * <command>: 'I' for insert, 'D' for delete, 'F' for find
   * <key>: uint64_t
   * <value>: int
   */
  storage::BPlusTree<storage::hash_t, int> bpt(&bpm);
  char command;
  storage::hash_t key;
  int value;
  int i = 0;
  while (std::cin >> command) {
    ++i;

    switch (command) {
      case 'I': {
        std::cin >> key >> value;
        bpt.Insert(key, value);
        break;
      }
      case 'D': {
        std::cin >> key;
        bpt.Remove(key);
        break;
      }
      case 'F': {
        std::cin >> key;
        value = -1;
        auto hint = bpt.GetValue(key, &value);
        if (hint) {
          std::cout << "Search " << key << ": " << value << std::endl;
        } else {
          std::cout << "Search " << key << ": Not Found" << std::endl;
        }
        break;
      }
      default:std::cerr << "Unknown command: " << command << std::endl;
    }
//    if (command != 'F' && i % 100 == 0) {
////      std::cerr << "Print B+ tree after operation " << i << ": " << command << " " << key << std::endl;
////      bpt.Print();
//      if (!bpt.Validate()) {
//        std::cerr << "Validation failed after " << i << " operations" << std::endl;
//        break;
//      }
//    }
  }
}

int main() {
  bool reset = true;
  storage::BufferPoolManager<1> bpm("test", reset);
  bpt_test(bpm);
  return 0;
}