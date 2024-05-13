//
// Created by zj on 4/27/2024.
//

#include "buffer_pool_manager.h"
#include "b_plus_tree.h"
#include "config.h"
#include <iostream>
#include <parser.h>

#include "fastio.h"
#include "hash.h"
#include "utility.h"
#include "variable_length_store.h"

void bpt_frame_test() {
  using internal_frame = storage::BPlusTreeInternalFrame<storage::hash_t, storage::page_id_t, 1>;
  using leaf_frame = storage::BPlusTreeLeafFrame<storage::hash_t, int, 1>;
  using leaf_frame2 = storage::BPlusTreeLeafFrame<char, char, 1>;
  std::cout << "Size of a frame with 1 page: " << storage::Frame<1>::kFrameSize << std::endl;
  std::cout << "Size of BPlusTreeInternalFrame<hash_t, page_id_t, 1>: " << sizeof(internal_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<hash_t, int, 1>: " << sizeof(leaf_frame) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<char, char, 1>: " << sizeof(leaf_frame2) << std::endl;

  using internal_frame2 = storage::BPlusTreeInternalFrame<storage::hash_t, storage::page_id_t, 2>;
  using leaf_frame3 = storage::BPlusTreeLeafFrame<storage::hash_t, int, 2>;
  using leaf_frame4 = storage::BPlusTreeLeafFrame<char, char, 2>;
  std::cout << "Size of a frame with 2 pages: " << storage::Frame<2>::kFrameSize << std::endl;
  std::cout << "Size of BPlusTreeInternalFrame<hash_t, page_id_t, 2>: " << sizeof(internal_frame2) << std::endl;
  std::cout << "Max Size of BPlusTreeInternalFrame<hash_t, page_id_t, 2>: " << internal_frame2::GetMaxSize() <<
      std::endl;
  std::cout << "(8192 - 8 - 4) / (8 + 4) " << (8192 - 4 - 4) / (8 + 4) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<hash_t, int, 2>: " << sizeof(leaf_frame3) << std::endl;
  std::cout << "Size of BPlusTreeLeafFrame<char, char, 2>: " << sizeof(leaf_frame4) << std::endl;
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
  static constexpr int pages_per_frame = storage::BPT_PAGES_PER_FRAME;
  storage::BufferPoolManager<pages_per_frame> bpm("test.db", reset);
  storage::page_id_t root_page_id = storage::INVALID_PAGE_ID;
  storage::BPlusTree<storage::hash_t, int> bpt(&bpm, root_page_id);
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
      default: std::cerr << "Unknown command: " << command << std::endl;
    }
    if (command != 'F' && i % 100 == 0) {
      //      std::cerr << "Print B+ tree after operation " << i << ": " << command << " " << key << std::endl;
      //      bpt.Print();
      // if (!bpt.Validate()) {
      //   std::cerr << "Validation failed after " << i << " operations" << std::endl;
      //   break;
      // }
    }
  }
}

void storage_test(bool force_reset = false) {
  /*
   * Input format (stdin):
   * first is the number of operations n
   * then n lines of operations, each line is in the format of
   * insert [index] [value]
   * delete [index] [value]
   * find [index]
   * [index] is a string, and [value] is an integer
   * [index] can duplicate, but pair(index, value) is unique
   * Output format (stdout):
   * `insert` and `delete` do not output anything
   * `find` outputs all values with the same index in a line, separated by space
   * if no value is found, output "null"
   */
  bool reset = force_reset;
  if (!reset) {
    std::ifstream file("test2.db");
    reset = !file.good();
  }
  static constexpr int pages_per_frame = storage::BPT_PAGES_PER_FRAME;
  storage::BufferPoolManager<pages_per_frame> bpm("test2.db", reset);
  int &bpt_root = bpm.GetInfo(1);
  if (reset) {
    bpt_root = storage::INVALID_PAGE_ID;
  }
  storage::BPlusTree<storage::PackedPair<storage::hash_t, int>, char> bpt(&bpm, bpt_root);
  storage::Hash hash;
  int n;
  using IO = utils::FastIO;
  IO::read(n);
  for (int i = 0; i < n; ++i) {
    std::string cmd, key_str;
    IO::read(cmd, key_str);
    auto key_hash = hash(key_str);
    if (cmd == "insert") {
      int value;
      IO::read(value);
      bpt.Insert({key_hash, value}, 'a');
    } else if (cmd == "delete") {
      int value;
      IO::read(value);
      bpt.Remove({key_hash, value});
    } else if (cmd == "find") {
      char value = 'a';
      auto result = bpt.PartialSearch(key_hash);
      for (const auto &item : result) {
        IO::write(item.first.second, ' ');
      }
      if (result.empty()) IO::write("null");
      IO::write('\n');
    } else {
      IO::write("Unknown command: ", cmd, '\n');
    }
  }
}

void pair_test() {
  //  std::cout << "Size of TrivialPair: " << sizeof (TrivialPair) << std::endl; // 16
  std::cout << "Size of PackedPair: " << sizeof(storage::PackedPair<storage::hash_t, int>) << std::endl; // 12
}

void parser_test() {
  std::string input = "[1623456789] command -a こんにちは -b value2";
  try {
    auto [command, args] = utils::Parser::Read(input);
    std::cout << "Command: " << command << '\n';
    std::cout << "Timestamp: " << args.GetTimestamp() << '\n';
    std::cout << "Flag -a: " << args.GetFlag('a') << '\n';
    std::cout << "Flag -b: " << args.GetFlag('b') << '\n';
  } catch (const std::invalid_argument &e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}

int main() {
  parser_test();
  return 0;
}