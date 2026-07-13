cmake_minimum_required(VERSION 3.10)
project(MemSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- 1. INCLUDE DIRECTORIES (CRITICAL FIX) ---
# This tells the compiler to look inside the 'include' folder
# so it can find "sim/clock.h"
include_directories(include)

# --- 2. FIND NCURSES ---
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# --- 3. SOURCE FILES ---
set(MEMSIM_CORE_SOURCES
  src/clock.cpp
  src/eventqueue.cpp
  src/workload.cpp
  src/process.cpp
  src/physicalmem.cpp
  src/MMU.cpp
  src/scheduler.cpp
  src/allocator.cpp
  src/metrics.cpp
  src/TLB.cpp
  src/timeline.cpp   
  src/TUI.cpp
)

# --- 4. BUILD CORE LIBRARY ---
add_library(memsim_core ${MEMSIM_CORE_SOURCES})

# Link Ncurses to the core library
target_link_libraries(memsim_core ${CURSES_LIBRARIES})

# --- 5. BUILD EXECUTABLE ---
add_executable(memsim_a src/main.cpp)

# Link the core library to the executable
target_link_libraries(memsim_a memsim_core)
