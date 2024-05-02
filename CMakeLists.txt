cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++17 support.

project(ACM_TicketSystem)


include_directories(src)
include_directories(third_party)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

file(GLOB_RECURSE main_src src/*.cpp )
file(GLOB_RECURSE third_party_src third_party//*.cpp )

list(REMOVE_ITEM main_src "${CMAKE_CURRENT_SOURCE_DIR}/src/b_plus_tree.cpp")

add_executable(code ${main_src} ${third_party_src})