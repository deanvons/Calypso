cmake_minimum_required(VERSION 3.10)
project(calypso C)
set(CMAKE_C_STANDARD 99)
add_executable(calypso main.c)

# Suppress MSVC's deprecation warnings for standard C library functions
# (scanf, printf, etc. are standard C99 — these warnings are MSVC-specific noise)
if(MSVC)
    target_compile_definitions(calypso PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
