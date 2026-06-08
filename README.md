# Calypso

Calypso is an interplanetary crew shuttle. This repo is the source code for its onboard flight computer — a bare-metal C system that monitors sensor readings, tracks mission state, manages crew records, and logs telemetry to persistent storage.

The project grows from a single `main.c` into a multi-file C system with dedicated modules for sensors, navigation, crew management, and hardware control. Each phase introduces a genuine operational requirement that the existing codebase cannot satisfy, and the C feature introduced in that phase is the natural answer.

See [docs/PRD.md](docs/PRD.md) for the full product brief.

---

## Prerequisites

- GCC or Clang (C99 or later)
- CMake 3.10 or later
- On Windows: MinGW-w64, MSVC (with C99 support), or WSL

---

## Build and run

```bash
mkdir build
cmake -B build
cmake --build build
./build/calypso       # Linux / macOS
.\build\Debug\calypso.exe   # Windows (MSVC)
.\build\calypso.exe         # Windows (MinGW)
```

---

## Branch sequence

| Branch | What it introduces | Abstraction level |
|---|---|---|
| `main` | Project scaffold — compiles and runs | Scaffold only |
| `phase-01_boot-and-io` | First program · `printf` / `scanf` · compilation model | Raw I/O |
| `phase-02_debugging` | `printf`-trace debugging · VS Code debugger · breakpoints | — |
| `phase-03_integer-types` | `stdint.h` fixed-width types · `PRIu16` format specifiers · overflow guards | — |
| `phase-04_compound-types` | `float` · `char` · `bool` · `enum MissionPhase` · `typedef` | — |
| `phase-05_operators` | Arithmetic · relational · logical · `sizeof` · explicit casts | — |
| `phase-06_bitwise` | Bitmasks · `ENGINE_CTRL` register · `1u << n` shift pattern | — |
| `phase-07_control-flow` | `while(1)` command loop · `switch` on mission phase · `goto` emergency shutdown | — |
| `phase-08_functions` | `sensors.c` / `engine.c` / `navigation.c` split · prototypes · pass-by-value | Modular |
| `phase-09_arrays` | Circular sensor history buffers · `sizeof` element count · `array[i]` ≡ `*(array + i)` | — |
| `phase-10_pointers` | `&` / `*` · pointer arithmetic · `const T*` vs `T* const` · `**` | — |
| `phase-11_strings` | `char` arrays · `strncpy` / `strcmp` / `strlen` · null terminator | — |
| `phase-12_structs` | `crew_member_t` · `spacecraft_t` · dot / arrow notation · nested structs | — |
| `phase-13_dynamic-memory` | `malloc` / `realloc` / `free` · dynamic crew roster · `NULL` checks | — |
| `phase-14_embedded-patterns` | `volatile` · memory-mapped I/O pointer · struct bitfields · `const` ROM data | Hardware abstraction |
| `phase-15_preprocessor` | `#define` constants · include guards · `#ifdef DEBUG_TELEMETRY` · function-like macro | — |
| `phase-16_file-io` | `fopen` / `fprintf` / `fwrite` · `calypso.log` · binary checkpoint | — |
