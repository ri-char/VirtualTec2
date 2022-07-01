cmake_minimum_required(VERSION 3.20)
project(monitor)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(monitor
        bit.cpp
        bit.h
        chip.cpp
        chip.h
        tec2.cpp
        tec2.h
        monitor.cpp)
target_link_libraries(monitor ${})
target_link_libraries(monitor ${CURSES_LIBRARY})
