cmake_minimum_required(VERSION 2.6)
project(GBPP)

find_package(SDL)
find_package(OpenGL)

set(CMAKE_CXX_FLAGS "-O3")

add_subdirectory(libgbpp)

add_executable(GBPP Main.cpp)
target_link_libraries(GBPP gbpp ${SDL_LIBRARY} ${OPENGL_LIBRARY})

#add_executable(GBPP MACOSX_BUNDLE Main.cpp Screen.cpp)
