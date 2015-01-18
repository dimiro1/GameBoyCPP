cmake_minimum_required(VERSION 2.6)
project(LIBGBPP)

set(LIBGBPP_VERSION_MAJOR 0)
set(LIBGBPP_VERSION_MINOR 0)
set(LIBGBPP_VERSION_PATH 1)

configure_file(
  "${PROJECT_SOURCE_DIR}/version.h.in"
  "${PROJECT_BINARY_DIR}/version.h"
)

set(CMAKE_CXX_FLAGS "-O3")
add_library(gbpp Memory.cpp Cartridge.cpp Lcd.cpp Cpu.cpp GameBoy.cpp)