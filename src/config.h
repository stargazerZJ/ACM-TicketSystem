//
// Created by zj on 4/27/2024.
//

#pragma once

#include <cstdint>
#include <limits>
namespace storage {

static constexpr int PAGE_SIZE = 4096;

using page_id_t = int32_t;
static constexpr page_id_t INVALID_PAGE_ID = -1;

//static constexpr int BPT_MAX_DEGREE = 3; // For testing purpose, will have no effect if set to infinity
static constexpr int BPT_MAX_DEGREE = std::numeric_limits<int>::max(); // For testing purpose, will have no effect if set to infinity

using hash_t = uint64_t;


}