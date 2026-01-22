cmake_minimum_required(VERSION 3.16)
project(memsim VERSION 0.1 LANGUAGES CXX)

# ---------------- Build options ----------------
option(BUILD_SELFTESTS "Build self-test executables for individual modules" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug/Release)" FORCE)
endif()

# ---------------- C++ standard ----------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------- Include dirs ----------------
set(MEMSIM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
include_directories(${MEMSIM_INCLUDE_DIR})

# ---------------- Core sources ----------------
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
)

# ---------------- Core library ----------------
add_library(memsim_core STATIC ${MEMSIM_CORE_SOURCES})
target_include_directories(memsim_core PUBLIC ${MEMSIM_INCLUDE_DIR})

# ---------------- CLI executable ----------------
add_executable(memsim_a src/main.cpp)
target_link_libraries(memsim_a memsim_core)

set_target_properties(memsim_a PROPERTIES
  OUTPUT_NAME "memsim_a"
)

# ---------------- Optional self-tests ----------------
if(BUILD_SELFTESTS)
  message(STATUS "Building module self-tests")

  add_executable(clock_test src/clock.cpp)
  target_compile_definitions(clock_test PRIVATE CLOCK_SELFTEST)
  target_include_directories(clock_test PRIVATE ${MEMSIM_INCLUDE_DIR})

  add_executable(eventqueue_test src/eventqueue.cpp)
  target_compile_definitions(eventqueue_test PRIVATE EVENTQUEUE_SELFTEST)
  target_include_directories(eventqueue_test PRIVATE ${MEMSIM_INCLUDE_DIR})

  add_executable(workload_test src/workload.cpp)
  target_compile_definitions(workload_test PRIVATE WORKLOAD_SELFTEST)
  target_include_directories(workload_test PRIVATE ${MEMSIM_INCLUDE_DIR})
endif()

# ---------------- GUI ----------------
add_subdirectory(external/imgui)

add_executable(memsim_gui
  src/gui_main.cpp
)

target_include_directories(memsim_gui PRIVATE
  ${MEMSIM_INCLUDE_DIR}
  external/imgui
  external/imgui/backends
)

target_link_libraries(memsim_gui
  memsim_core
  imgui
  glfw
  OpenGL::GL
)

# ---------------- Install ----------------
install(TARGETS memsim_a memsim_gui
        RUNTIME DESTINATION bin)

# ---------------- Summary ----------------
message(STATUS "memsim project configured")
message(STATUS "  Source dir : ${CMAKE_SOURCE_DIR}")
message(STATUS "  Include dir: ${MEMSIM_INCLUDE_DIR}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Self-tests : ${BUILD_SELFTESTS}")
