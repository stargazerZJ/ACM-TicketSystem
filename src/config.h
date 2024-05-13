//
// Created by zj on 4/27/2024.
//

#pragma once

#include <cstdint>
#include <limits>
#include <iostream> // for debugging
namespace storage {

static constexpr int PAGE_SIZE = 4096;

using page_id_t = int32_t;
static constexpr page_id_t INVALID_PAGE_ID = -1;

//static constexpr int BPT_MAX_DEGREE = 100; // For testing purpose, will have no effect if set to infinity
static constexpr int BPT_MAX_DEGREE = std::numeric_limits<int>::max(); // For testing purpose, will have no effect if set to infinity
static constexpr int BPT_PAGES_PER_FRAME = 1;

using record_id_t = int32_t;
static constexpr int VLS_PAGES_PER_FRAME = 1;

using hash_t = uint64_t;

static constexpr int LRU_REPLACER_K = 10;
static constexpr int BUFFER_POOL_SIZE = 1000;

} // namespace storage