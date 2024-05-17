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
static constexpr record_id_t INVALID_RECORD_ID = -1;
static constexpr int VLS_PAGES_PER_FRAME = 1;

using hash_t = uint64_t;

static constexpr int LRU_REPLACER_K = 20;
static constexpr int BUFFER_POOL_SIZE = 2500;

static constexpr char DB_FILE_NAME[] = "db.bin";

} // namespace storage

namespace business {
using date_t = int8_t; // 0 ~ 91 (0: 06-01, 91: 08-31)
using time_t = int16_t; // 0 ~ 1439 (0: 00:00, 1439: 23:59)
using abs_time_t = int32_t; // 0 ~ 92 * 1440 - 1, (0: 06-01 00:00, 92 * 1440 - 1: 08-31 23:59)
using order_no_t = int16_t; // 0 ~ 32767

static constexpr date_t MAX_DATE = 92;
static constexpr size_t DATE_BATCH_SIZE = 10; // the number of days in a Vacancy var_length_object
static constexpr size_t DATE_BATCH_COUNT = (MAX_DATE + DATE_BATCH_SIZE - 1) / DATE_BATCH_SIZE; // the number of Vacancy var_length_object in TrainInfo

} // namespace business
