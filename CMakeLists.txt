cmake_minimum_required(VERSION 3.5)

project(robin_map LANGUAGES C)

add_compile_options(-std=c89 -ansi -Og -Wall -Wextra -Wpedantic -DNDEBUG)

add_executable(robin_map robin_map.h robin_map.c main.c)
