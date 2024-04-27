//
// Created by zj on 4/27/2024.
//

#include "buffer_pool_manager.h"

int main() {
  bool reset = true;
  storage::BufferPoolManager<1> bpm("test.db", reset);
  return 0;
}