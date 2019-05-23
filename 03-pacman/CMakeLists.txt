cmake_minimum_required(VERSION 2.8)

project(pacman C)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
add_compile_options(-std=c11 -Wall -Wextra -gdwarf-2 -g3 -fno-omit-frame-pointer)

add_library(game SHARED src/game.c)
target_link_libraries(game m)

add_executable(platform src/linux_x11.c)
target_link_libraries(platform X11 Xext dl)
