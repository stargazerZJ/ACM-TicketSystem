//
// Created by zj on 4/27/2024.
//

#pragma once

#include <cstdint>
namespace storage {

static constexpr int PAGE_SIZE = 4096;

using page_id_t = int32_t;
static constexpr page_id_t INVALID_PAGE_ID = -1;


}