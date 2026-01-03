cmake_minimum_required(VERSION 3.16)
project(memsim VERSION 0.1 LANGUAGES CXX)

# User-tweakable options
option(BUILD_SELFTESTS "Build small self-test executables for individual modules" OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug/Release)" FORCE)
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Basic warnings (non-fatal)
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
set(MEMSIM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
include_directories(${MEMSIM_INCLUDE_DIR})

# Source files for milestone A
set(MEMSIM_SOURCES
    src/clock.cpp
    src/eventqueue.cpp
    src/workload.cpp
    src/process.cpp
    src/physicalmem.cpp
    src/MMU.cpp
    src/main.cpp
    src/scheduler.cpp
    src/allocator_firstfit.cpp
    src/metrics.cpp
    src/TLB.cpp
)


# Main executable
add_executable(memsim ${MEMSIM_SOURCES})
target_include_directories(memsim PRIVATE ${MEMSIM_INCLUDE_DIR})

# Small helper target names and properties
set_target_properties(memsim PROPERTIES
    OUTPUT_NAME "memsim_a"
)

# Optional self-tests (each compiles only the module that contains a SELFTEST guard)
if(BUILD_SELFTESTS)
    message(STATUS "Building module self-tests (CLOCK_SELFTEST, EVENTQUEUE_SELFTEST, WORKLOAD_SELFTEST)")
    add_executable(clock_test src/Clock.cpp)
    target_compile_definitions(clock_test PRIVATE CLOCK_SELFTEST)
    target_include_directories(clock_test PRIVATE ${MEMSIM_INCLUDE_DIR})

    add_executable(eventqueue_test src/EventQueue.cpp)
    target_compile_definitions(eventqueue_test PRIVATE EVENTQUEUE_SELFTEST)
    target_include_directories(eventqueue_test PRIVATE ${MEMSIM_INCLUDE_DIR})

    add_executable(workload_test src/Workload.cpp)
    target_compile_definitions(workload_test PRIVATE WORKLOAD_SELFTEST)
    target_include_directories(workload_test PRIVATE ${MEMSIM_INCLUDE_DIR})
endif()

# Install rules (optional)
install(TARGETS memsim
        RUNTIME DESTINATION bin)

# Useful summary when configuring
message(STATUS "memsim project configured")
message(STATUS "  Source dir : ${CMAKE_SOURCE_DIR}")
message(STATUS "  Include dir: ${MEMSIM_INCLUDE_DIR}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Self-tests : ${BUILD_SELFTESTS}")

# End of CMakeLists.txt
