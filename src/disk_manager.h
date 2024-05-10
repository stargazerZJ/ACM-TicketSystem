//
// Created by zj on 5/10/2024.
//

#pragma once
#include <config.h>
#include <cstring>
#include <fstream>
#include <utility>
#include "marcos.h"

namespace storage {
template<int PagesPerFrame>
class DiskManager {
  public:
    explicit DiskManager(std::string db_file, bool reset);
    ~DiskManager();
    void ShutDown();
    void WriteFrame(page_id_t page_id, const char *page_data);
    void ReadFrame(page_id_t page_id, char *page_data);
    unsigned int AllocateFrame();
    void DeallocateFrame(page_id_t page_id);
    int &GetInfo(int index);

  private:
    static constexpr int kFrameSize = PAGE_SIZE * PagesPerFrame;
    static constexpr int kInfoSize = PAGE_SIZE / sizeof(int);
    using InfoPage = int[kInfoSize];
    std::string db_file_;
    std::fstream db_io_;
    int size_; // number of frames
    InfoPage info_page_{};
    int &free_head = info_page_[0];

    static int toOffset(page_id_t page_id);
};
template<int PagesPerFrame>
DiskManager<PagesPerFrame>::DiskManager(std::string db_file, bool reset) : db_file_(std::move(db_file)) {
  if (reset) {
    db_io_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    memset(info_page_, 0, sizeof(InfoPage));
    db_io_.write(reinterpret_cast<char *>(info_page_), sizeof(InfoPage));
    size_ = 0;
    free_head = INVALID_PAGE_ID;
  } else {
    db_io_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary);
    db_io_.read(reinterpret_cast<char *>(info_page_), sizeof(InfoPage));
    db_io_.seekg(0, std::ios::end);
    size_ = (db_io_.tellg() / PAGE_SIZE - 1) / PagesPerFrame;
  }
  if (!db_io_.is_open()) {
    throw std::runtime_error("Cannot open file " + db_file_);
  }
}
template<int PagesPerFrame>
DiskManager<PagesPerFrame>::~DiskManager() {
  ShutDown();
}
template<int PagesPerFrame>
void DiskManager<PagesPerFrame>::ShutDown() {
  db_io_.seekp(0, std::ios::beg);
  db_io_.write(reinterpret_cast<char *>(info_page_), sizeof(InfoPage));
  db_io_.close();
}
template<int PagesPerFrame>
void DiskManager<PagesPerFrame>::WriteFrame(page_id_t page_id, const char *page_data) {
  ASSERT(page_id >= 0 && page_id < size_);
  db_io_.seekp(toOffset(page_id));
  db_io_.write(page_data, kFrameSize);
}
template<int PagesPerFrame>
void DiskManager<PagesPerFrame>::ReadFrame(page_id_t page_id, char *page_data) {
  ASSERT(page_id >= 0 && page_id < size_);
  db_io_.seekg(toOffset(page_id));
  db_io_.read(page_data, kFrameSize);
}
template<int PagesPerFrame>
unsigned int DiskManager<PagesPerFrame>::AllocateFrame() {
  if (free_head == INVALID_PAGE_ID) {
    return size_ ++;
  }
  int new_free_head = INVALID_PAGE_ID;
  int ret = free_head;
  db_io_.seekg(toOffset(free_head));
  db_io_.read(reinterpret_cast<char *>(&new_free_head), sizeof(int));
  free_head = new_free_head;
  return ret;
}
template<int PagesPerFrame>
void DiskManager<PagesPerFrame>::DeallocateFrame(page_id_t page_id) {
  ASSERT(page_id >= 0 && page_id < size_);
  int old_free_head = free_head;
  free_head = page_id;
  db_io_.seekp(toOffset(page_id));
  db_io_.write(reinterpret_cast<char *>(&old_free_head), sizeof(int));
}
template<int PagesPerFrame>
int &DiskManager<PagesPerFrame>::GetInfo(int index) {
  ASSERT(index > 0 && index < kInfoSize);
  return info_page_[index];
}
template<int PagesPerFrame>
int DiskManager<PagesPerFrame>::toOffset(page_id_t page_id) {
  return page_id * kFrameSize + sizeof(InfoPage);
}
} // namespace storage
