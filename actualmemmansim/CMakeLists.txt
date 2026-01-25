cmake_minimum_required(VERSION 3.10)
project(MemSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Fixes the DT_TEXTREL warning

# --- 1. FIND NCURSES ---
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# --- 2. SOURCE FILES ---
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
  src/TUI.cpp        # Must match casing on disk
)

# --- 3. BUILD CORE LIBRARY ---
add_library(memsim_core ${MEMSIM_CORE_SOURCES})

# CRITICAL FIX: Link Ncurses directly to the library that uses it (memsim_core)
target_link_libraries(memsim_core ${CURSES_LIBRARIES})

# --- 4. BUILD EXECUTABLE ---
add_executable(memsim_a src/main.cpp)

# Link the core library to the executable
target_link_libraries(memsim_a memsim_core)
