//
// Created by zj on 5/10/2024.
//

#pragma once

#include <cassert>

#ifdef DEBUG
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

#define DELETE_CONSTRUCTOR_AND_DESTRUCTOR(class_name) class_name() = delete;  ~class_name() = delete;