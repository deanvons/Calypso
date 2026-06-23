cmake_minimum_required(VERSION 3.10)
project(calypso C)
set(CMAKE_C_STANDARD 99)
add_executable(calypso main.c sensors.c engine.c navigation.c crew.c log.c)

# Suppress MSVC's deprecation warnings for standard C library functions
# (scanf, printf, etc. are standard C99 — these warnings are MSVC-specific noise)
if(MSVC)
    target_compile_definitions(calypso PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# CALYPSO_DEBUG_TELEMETRY: configure-time switch for the DEBUG_TELEMETRY macro
# (see sensors.h / main.c). OFF by default; enable with:
#   cmake -B build -DCALYPSO_DEBUG_TELEMETRY=ON
option(CALYPSO_DEBUG_TELEMETRY "Enable debug telemetry output" OFF)

if(CALYPSO_DEBUG_TELEMETRY)
    target_compile_definitions(calypso PRIVATE DEBUG_TELEMETRY)
endif()
