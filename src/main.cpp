//
// Created by zj on 4/27/2024.
//

#include "buffer_pool_manager.h"
#include "b_plus_tree.h"
#include "config.h"
#include <iostream>
#include "fastio.h"
#include "hash.h"

void bpt_frame_test() {
  using internal_frame = storage::BPlusTreeInternalFrame<storage::hash_t, storage::page_id_t, 1>;
  using leaf_frame = storage::BPlusTreeLeafFrame<storage::hash_t, int, 1>;
  using leaf_frame2 = storage::BPlusTreeLeafFrame<char, char, 1>;
  std::cout << "Size of a frame with 1 page: " << storage::Frame<1>::kFrameSize << std::endl;
  std::cout << "Size of BPlusTreeInternalFrame<hash_t, page_id_t, 1>: " << sizeof(internal_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<hash_t, int, 1>: " << sizeof(leaf_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<char, char, 1>: " << sizeof(leaf_frame2) << std::endl;
}

void bpt_test() {
  /*
   * Input format (stdin):
   * <command> <key> <value>
   * <command>: 'I' for insert, 'D' for delete, 'F' for find
   * <key>: uint64_t
   * <value>: int
   */
  bool reset = true;
  storage::BufferPoolManager<1> bpm("test", reset);
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
    if (command != 'F' && i % 100 == 0) {
//      std::cerr << "Print B+ tree after operation " << i << ": " << command << " " << key << std::endl;
//      bpt.Print();
      if (!bpt.Validate()) {
        std::cerr << "Validation failed after " << i << " operations" << std::endl;
        break;
      }
    }
  }
}

void storage_test() {
  /*
   * Input format (stdin):
   * insert [index] [value]
   * delete [index]
   * find [index]
   * [index] is a string, and [value] is an integer
   * [index] can duplicate, but pair(index, value) is unique
   */
  bool reset = false;
  {
    std::ifstream file("test.db");
    reset = !file.good();
  }
  storage::BufferPoolManager<1> bpm("test", reset);
  int &bpt_header = bpm.getInfo(1);
  if (reset) {
    bpt_header = storage::INVALID_PAGE_ID;
  }
  storage::BPlusTree<std::pair<storage::hash_t, int>, char> bpt(&bpm, bpt_header);
  storage::Hash hash;
  int n;
  using IO = fastio::FastIO;
  IO::read(n);
  for (int i = 0; i < n; ++i) {
    std::string cmd, key_str;
    IO::read(cmd, key_str);
    auto key_hash = hash(key_str);
    if (cmd == "insert") {
      int value;
      IO::read(value);
      bpt.Insert(std::make_pair(key_hash, value), 'a');
    } else if (cmd == "delete") {
      bpt.RemoveAll(key_hash);
    } else if (cmd == "find") {
      char value = 'a';
      auto result = bpt.PartialSearch(key_hash);
      for (const auto &item : result) {
        IO::write(item.second, ' ');
      }
      if (result.empty()) IO::write("null");
      IO::write('\n');
    } else {
      IO::write("Unknown command: ", cmd, '\n');
    }
  }
}

int main() {
  return 0;
}