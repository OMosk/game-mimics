cmake_minimum_required(VERSION 2.8)

project(snake CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(CMAKE_REQUIRED_FLAGS -std=c++14)
add_compile_options(-Wall -Wextra -Werror -pedantic -fno-exceptions)

add_executable(snake src/linux_x11.cpp src/game.cpp)
target_link_libraries(snake X11)
