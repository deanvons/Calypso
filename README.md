# Phase README — Build configuration

> **Phase 15 — Preprocessor and macros** | Calypso · Core C

Replacing duplicated literals with named `#define` constants, gating debug output with conditional compilation, introducing a function-like macro that reports its own call site, and adding include guards to every header.

The engine register base address, the sensor fault-range thresholds, and the crew name buffer size all live as numeric literals scattered through the codebase — some already pulled into a `#define`, most still typed out by hand wherever they're needed. Phase 14 left two open questions pointing straight here: the register base address `0x40020000UL` lives in one place today, but if a second file ever needed it, you'd be copying a number with no name attached to it; and the boot-time fault checks and the periodic sensor scan both hard-code the same pressure and velocity thresholds, so changing one means remembering to change the other. Neither problem is fixed by anything you've learned so far — `const` gives you a read-only *variable*, but a variable still has an address and a type, and you can't use it to size an array or guard a header. You need something that runs *before* the compiler sees your code at all.

You've also been quietly relying on the preprocessor since Phase 1 — `#include` has been splicing header files into every source file this whole time, and `__DATE__` has been printing the boot banner's build date since Phase 14 — without ever looking at what either one is actually doing. This phase makes the preprocessor's role explicit: it is a separate, text-only pass, and that has consequences for what it can and can't check.

> **A note on scope.** This phase does not touch the build system. CMake's `target_compile_definitions` and command-line `-D` flags are how a real project would normally toggle `DEBUG_TELEMETRY` per build configuration — here you'll set it by hand to keep the focus on what the preprocessor does with it, not on CMake.

---

## 🗺️ Contents

- [Branch sequence](#-branch-sequence)
- [Solutions to previous challenges and thought pieces](#-solutions-to-previous-challenges-and-thought-pieces)
- [Why we made this decision](#-why-we-made-this-decision)
- [What we built in the previous branch](#-what-we-built-in-the-previous-branch)
- [What we're doing in this branch](#-what-were-doing-in-this-branch)
- [Learning goals](#-learning-goals)
- [Key concepts](#-key-concepts)
- [What to notice in the code](#-what-to-notice-in-the-code)
- [What this phase revealed](#-what-this-phase-revealed)
- [Running this branch](#-running-this-branch)
- [Challenges for students](#-challenges-for-students)
- [Thought pieces for the next branch](#-thought-pieces-for-the-next-branch)

---

## 📍 Branch sequence

| Branch | What it introduces | Abstraction level |
|---|---|---|
| `main` | Project scaffold — compiles and runs | Scaffold only |
| `phase-01_boot-and-io` | First program · `printf` / `scanf` · compilation model | Raw I/O |
| `phase-02_debugging` | `printf` tracing · VS Code debugger · three C error categories | — |
| `phase-03_integer-types` | `stdint.h` fixed-width types · `PRIu16` format specifiers · overflow guards | — |
| `phase-04_compound-types` | `float` / `double` · `bool` · `enum MissionPhase` · `typedef sensor_float_t` | — |
| `phase-05_operators` | Arithmetic · relational · logical · `sizeof` · explicit casts | — |
| `phase-06_bitwise` | Bitmasks · `ENGINE_CTRL` register · `1u << n` shift pattern | — |
| `phase-07_control-flow` | `while(1)` command loop · `switch` state machine · `goto` emergency shutdown | — |
| `phase-08_functions` | `sensors.c` / `engine.c` / `navigation.c` split · prototypes · pass-by-value | Modular |
| `phase-09_arrays` | Sensor history buffers · `sizeof` element count · `array[i]` ≡ `*(array + i)` | — |
| `phase-10_pointers` | `&` / `*` · in-place calibration · pointer arithmetic · `const T*` vs `T* const` · `**` | — |
| `phase-11_strings` | `char` arrays · null terminator · `strncpy` / `strcmp` / `strlen` / `strncat` · literal vs mutable | — |
| `phase-12_structs` | `crew_member_t` · `spacecraft_t` · dot / arrow notation · nested structs · array of structs | — |
| `phase-13_dynamic-memory` | `malloc` / `realloc` / `free` · dynamic crew roster · `NULL` checks · mission log buffer | — |
| `phase-14_embedded-patterns` | `volatile` · memory-mapped I/O pointer · struct bitfields · `const` ROM data | Hardware abstraction |
| `📌 phase-15_preprocessor` | **`#define` constants · `#ifdef DEBUG_TELEMETRY` · `ASSERT_SENSOR_RANGE` macro · include guards** | Build-time configuration |
| `phase-16_file-io` | `fopen` / `fprintf` / `fwrite` · `calypso.log` · binary checkpoint | — |

---

## ✅ Solutions to previous challenges and thought pieces

### How challenges work

Additive challenges from the previous branch are solved in the first commit of this
branch — look for the `SOLUTION:` commit at the top of this branch's git history.
Analytical challenges and thought pieces are answered below.

```bash
git log --oneline          # find the SOLUTION commit hash
git show <hash>            # inspect the solution in isolation
```

### Challenge 1 — What the program would see without `volatile`, and why the simulation hides it

Without `volatile`, a compiler optimizing the `while (1)` command loop is allowed to read `*pENGINE_CTRL` once, hold the value in a CPU register, and reuse that register for every later call to `engine_fault_critical()` — it has proven that nothing in the loop body writes to `ENGINE_CTRL`, so re-reading memory would (from its point of view) be a wasted instruction. On real hardware, the engine peripheral controller writes to the physical register between loop iterations, so the program would keep testing a frozen, stale copy of the register and never notice a fault the hardware had already raised. You don't observe this on the desktop because nothing in this simulation writes to `ENGINE_CTRL` except this program itself — the optimization the compiler would apply is always correct here, so adding or removing `volatile` produces identical behavior. The keyword changes a guarantee about generated machine code, not about program output on this platform.

### Challenge 2 — What breaks if `engine_ctrl_reg_t` were compiled for a big-endian MCU

The C standard leaves bitfield packing order, padding, and which end of the storage unit the first-declared field occupies entirely up to the compiler. `thrusters : 4` landing in bits 0–3 and `throttle : 4` landing in bits 4–7 is GCC/Clang's behavior on little-endian targets — not a promise the language makes. On a big-endian MCU where a different compiler packs bitfields from the most-significant bit down, `bits.throttle` could read a completely different four bits than `engine_read_throttle()`'s `(reg >> 4) & 0xF` — the two would silently disagree with no compiler warning. Projects that need a bitfield struct to behave identically across compilers typically don't rely on the compiler's packing decision at all: they verify the layout with a `static_assert` on `sizeof`, write target-specific bitfield structs guarded by `#ifdef`, or abandon bitfields for explicit shift-and-mask accessor functions, which behave identically everywhere because the arithmetic is fully specified by the standard.

### Challenge 4 — Two reasons `malloc` is risky in a hard real-time embedded context

First, allocation time is non-deterministic: `malloc` searches a free list whose size and fragmentation state vary at runtime, so one call might return in a few instructions and another might take orders of magnitude longer — a real-time system with a hard deadline cannot tolerate that variance. Second, heap fragmentation accumulates over a long-running system's lifetime: even when the total free memory is more than enough for a new allocation, no single free block may be large enough to satisfy it, so a `malloc` call that succeeded yesterday can fail today with no change in the request itself. Both properties are specific to how a general-purpose allocator manages memory — not to the absence of an OS — which is why some embedded projects that do have heap support still forbid `malloc` after startup.

### Thought piece 1 — One definition for `0x40020000UL`

`#define ENGINE_CTRL_BASE 0x40020000UL` is exactly the tool: it names the address once, and every place that needs it — right now just the demonstrative MMIO pointer in `engine.c`, but potentially any file that includes `engine.h` — refers to the name instead of retyping the literal. If the hardware team remaps the register in a board revision, you change the one `#define` and recompile; you are not searching the codebase for every spot that happened to type `0x40020000UL` by hand. This phase makes that change.

### Thought piece 2 — Toggling debug output without deleting and re-adding lines

`#ifdef DEBUG_TELEMETRY` / `#endif` wraps the debug-only output so the preprocessor includes it in the compiled program only when `DEBUG_TELEMETRY` is defined — undefined, and the lines between the guards are stripped from the source before the compiler ever sees them, with zero runtime cost. You flip it by defining the macro at compile time (`-DDEBUG_TELEMETRY` on the command line, or a `target_compile_definitions` entry in CMake) rather than editing the source at all. This phase wraps a block of telemetry output in exactly this guard.

### Thought piece 3 — What happens when a header is included twice in one translation unit

`#include` is a literal text-splice: the preprocessor deletes the `#include` line and pastes the named file's contents in its place, every time it sees the directive. If `main.c` includes `sensors.h` directly, and also includes `navigation.h`, which itself contains `#include "sensors.h"`, the preprocessor pastes `sensors.h`'s full contents into `main.c`'s translation unit twice. The compiler then sees `typedef float sensor_float_t;` and the `engine_ctrl_reg_t`-style declarations a second time — a redefinition, which is a compile error for most declaration forms. This phase adds the include guard that prevents it.

---

## 💡 Why we made this decision

### The preprocessor — text substitution before the compiler ever runs

Every `#include`, `#define`, and `#ifdef` in Calypso's source is resolved by a separate pass that runs before the compiler proper ever sees the code — the same preprocessor that has been splicing your header files together since Phase 8, and printing `__DATE__` into the boot banner since Phase 14. That pass has no concept of C types, scope, or syntax; it operates purely on text. `#define ENGINE_CTRL_BASE 0x40020000UL` does not declare a variable of any type — it tells the preprocessor "replace every later occurrence of the identifier `ENGINE_CTRL_BASE` with the text `0x40020000UL`," and the compiler that runs afterward never even knows a macro was involved. `#ifdef DEBUG_TELEMETRY` doesn't evaluate a runtime condition — it asks the preprocessor "has this name been `#define`d," and if not, deletes the guarded text before compilation, leaving nothing behind for the compiler to skip over at runtime.

```mermaid
flowchart LR
    SRC["engine.c\n#define ENGINE_CTRL_BASE ...\n#include \"engine.h\"\n#ifdef DEBUG_TELEMETRY ... #endif"] --> PP["Preprocessor\ntext substitution only -- no types, no scope"]
    PP -->|"expands macros\nsplices headers\nstrips unmatched #ifdef blocks"| TU["Expanded translation unit\n(plain C -- no macros or directives remain)"]
    TU --> CC["Compiler\ntype-checks and compiles the expanded text"]
```

### Symbolic constants over repeated literals

`0x40020000UL` exists once in the codebase today, as a comment in `engine.c`. The sensor fault-range thresholds are not so lucky: `80.0f` and `120.0f` for cabin pressure, and `0.0f` and `25.0f` for velocity, are each typed out twice in `main.c` — once at boot, once again inside the periodic sensor scan. Nothing connects those two call sites; if you needed to widen the pressure tolerance, you would have to remember both locations and update them in lockstep, and the compiler gives you no warning if you miss one. Naming the value once in a `#define` and writing the name at both call sites makes that link explicit — there is exactly one place where the threshold is decided, and both checks read from it.

### A macro, not a function — why `ASSERT_SENSOR_RANGE` needs to expand at the call site

`ASSERT_SENSOR_RANGE` reports the file and line number where an out-of-range reading was detected, using the predefined macros `__FILE__` and `__LINE__`. A function can't do this: if `assert_sensor_range()` were an ordinary function called from ten places in `main.c`, `__FILE__` and `__LINE__` inside its body would always expand to the one file and line where the function itself is *defined* — every call would report the same location, regardless of which call site actually triggered it. A macro doesn't have this problem, because it has no body of its own to report from: the preprocessor pastes the macro's text, `__FILE__`, `__LINE__`, and all, directly into each call site before the compiler ever runs, so `__FILE__` and `__LINE__` are evaluated fresh at every single place `ASSERT_SENSOR_RANGE` appears.

---

## ⏮️ What we built in the previous branch

Phase 14 qualified `ENGINE_CTRL` and `ENGINE_STATUS` `volatile`, switched `pENGINE_CTRL` to a `volatile`-qualified pointer, added a commented memory-mapped I/O pointer declaration demonstrating the cast-from-a-fixed-address pattern, introduced `engine_ctrl_reg_t` to read register fields as named bitfields instead of shift-and-mask arithmetic, and added a `const uint8_t BOOT_CONFIG[]` array annotated for ROM placement. The SOLUTION commit at the start of this branch adds `engine_halt()` / `engine_is_halted()` (Challenge 3) — a `volatile bool` flag the command loop checks on every iteration so the mission halts automatically on reaching `DOCKED` — and a bitfield printout after `engine_set_throttle(7)` (Challenge 5, stretch) confirming `bits.throttle` matches `engine_read_throttle()`.

---

## 🎯 What we're doing in this branch

- Add `#ifndef` / `#define` / `#endif` include guards to `engine.h`, `sensors.h`, `navigation.h`, and `crew.h`
- Add `#define ENGINE_CTRL_BASE 0x40020000UL` in `engine.h`; update the commented MMIO pointer declaration in `engine.c` to reference it instead of the raw literal
- Add symbolic sensor fault-range constants in `sensors.h` — `SENSOR_VELOCITY_FAULT_LOW`/`HIGH` and `SENSOR_PRESSURE_FAULT_LOW`/`HIGH` — and replace the two duplicated literal pairs in `main.c`'s boot-time checks and periodic scan
- Add the function-like macro `ASSERT_SENSOR_RANGE(val, min, max)` in `sensors.h`, using `__FILE__` and `__LINE__` to report where an out-of-range reading was detected; call it once for the velocity reading and once for the pressure reading in `main.c`
- Wrap a block of debug-only telemetry output in the periodic sensor scan (`main.c`, the `'s'` command) in `#ifdef DEBUG_TELEMETRY` / `#endif` — silent by default, compiled in only when the macro is defined

---

## 🧑🏻‍🏫 Learning goals

### Understand
- **Explain** the C preprocessor as a text-substitution pass that runs before compilation and has no knowledge of C types or scope — applied to why `ASSERT_SENSOR_RANGE` expands inline wherever it's called rather than being type-checked the way a function call would be
- **Identify** `__FILE__`, `__LINE__`, and `__DATE__` as predefined macros and what each expands to — `__DATE__` has been printing the boot banner's build date since Phase 14; `__FILE__` and `__LINE__` now appear inside `ASSERT_SENSOR_RANGE`'s expansion

### Apply
- **Use** `#define` to name `ENGINE_CTRL_BASE` and the sensor fault-range thresholds instead of repeating numeric literals across `main.c`
- **Define** `ASSERT_SENSOR_RANGE(val, min, max)` as a function-like macro and trace what it expands to at a specific call site in `main.c`
- **Use** `#ifdef DEBUG_TELEMETRY` to compile a block of sensor telemetry output in or out without touching the surrounding code
- **Write** `#ifndef` / `#define` / `#endif` include guards in `engine.h`, `sensors.h`, `navigation.h`, and `crew.h`

### Analyze
- **Examine** what the preprocessor produces from `engine.c` before the compiler ever sees it — trace how `#include`, `#define`, and `#ifdef` each transform the source text differently
- **Compare** `ASSERT_SENSOR_RANGE` as a macro against an equivalent `assert_sensor_range()` function — what the macro gains by expanding at the call site, and what it gives up by skipping the compiler's normal type checking

---

## 🔑 Key concepts

| Concept | Plain English |
|---|---|
| **The C preprocessor** | A text-substitution pass that runs before the compiler ever sees the code. It has no concept of C syntax, types, or scope — it only replaces and deletes text. |
| **Object-like macro** | `#define NAME value` — every later occurrence of `NAME` in the source is replaced with `value`, verbatim, with no type checking. |
| **Function-like macro** | `#define NAME(args) ...` — expands with its arguments substituted into the macro body at every call site, before the compiler runs. |
| **`__FILE__` / `__LINE__` / `__DATE__`** | Predefined macros the preprocessor expands to the current source file name, current line number, and compilation date — useful for diagnostics that must report exactly where they were triggered. |
| **Conditional compilation** | `#ifdef` / `#ifndef` / `#endif` blocks that include or exclude source text depending on whether a macro is defined. The excluded branch is deleted before the compiler runs — it isn't skipped at runtime, it never exists in the compiled program. |
| **Include guard** | An `#ifndef HEADER_H` / `#define HEADER_H` / `#endif` wrapper around a header's contents that stops the same declarations from being pasted into one translation unit twice. |

---

## 🔍 What to notice in the code

**[`engine.h`](engine.h)**
`ENGINE_CTRL_BASE` is now a named `#define` instead of a literal living only in a comment. The whole file is wrapped in an `#ifndef ENGINE_H` / `#define ENGINE_H` / `#endif` include guard.

**[`engine.c`](engine.c)**
The commented demonstrative MMIO pointer declaration now reads `(volatile uint32_t *)ENGINE_CTRL_BASE` instead of the raw hex literal — the same address, named once.

**[`sensors.h`](sensors.h)**
`SENSOR_VELOCITY_FAULT_LOW`/`HIGH` and `SENSOR_PRESSURE_FAULT_LOW`/`HIGH` replace the duplicated literal pairs from `main.c`. `ASSERT_SENSOR_RANGE(val, min, max)` is defined here as a function-like macro — read it alongside the `#define SENSOR_HISTORY_LEN 10` already present from Phase 9, which is the same mechanism you've been using since before this phase named it. The whole file is wrapped in an include guard.

**[`main.c`](main.c)**
The boot-time fault checks and the periodic scan's fault checks now both read from the same named constants instead of two independent sets of literals. `ASSERT_SENSOR_RANGE` is called once for the velocity reading and once for the pressure reading. The debug-only telemetry block in the `'s'` command is wrapped in `#ifdef DEBUG_TELEMETRY` / `#endif`.

**[`navigation.h`](navigation.h) · [`crew.h`](crew.h)**
Both now have include guards. Neither file's declarations changed otherwise.

---

## 🔗 What this phase revealed

> **LEARNING MOMENT:** `ASSERT_SENSOR_RANGE(val, min, max)` substitutes `val` into its expansion wherever the macro body references it — and the body references `val` twice, once for the low-bound comparison and once for the high-bound comparison. That's harmless as long as `val` is a plain variable, which is the only thing this codebase ever passes in. But if `val` were an expression with a side effect — `ASSERT_SENSOR_RANGE(sensors_read_velocity(), ...)` instead of `ASSERT_SENSOR_RANGE(velocity, ...)` — `sensors_read_velocity()` would run twice, silently, because the preprocessor has no concept of "evaluate this once and reuse the result" the way a function call does. The macro itself does nothing to enforce passing a plain variable; that discipline lives entirely in how you choose to call it.

---

## ▶️ Running this branch

**Prerequisites:** GCC or Clang (C99+) and CMake 3.10+, or just GCC/Clang directly.

**With CMake:**
```bash
cmake -B build
cmake --build build
.\build\Debug\calypso.exe   # Windows (MSVC)
.\build\calypso.exe         # Windows (MinGW)
./build/calypso             # Linux / macOS
```

**Direct compilation (no CMake):**
```bash
gcc -std=c99 main.c sensors.c engine.c navigation.c crew.c -o calypso
./calypso
```

**With debug telemetry enabled** — add `-DDEBUG_TELEMETRY` to either build:
```bash
gcc -std=c99 -DDEBUG_TELEMETRY main.c sensors.c engine.c navigation.c crew.c -o calypso
```
or, with CMake:
```bash
cmake -B build -DCMAKE_C_FLAGS=-DDEBUG_TELEMETRY
cmake --build build
```
With the flag defined, the `'s'` command's periodic sensor scan prints an additional debug line of raw readings. Without it, that line does not appear — and is not compiled into the binary at all, not merely hidden at runtime.

| Command | Action |
|---|---|
| `n` | Advance mission phase (halts the command loop automatically on reaching `DOCKED`) |
| `s` | Sensor scan — history, averages, drift, channel reconfiguration |
| `m` | Print crew manifest with loaded/capacity counts |
| `e` | Emergency shutdown — frees all allocations before exit |
| `q` | Normal quit — frees all allocations before exit |

---

## ✏️ Challenges for students

**Challenge 1 — Analytical**
`ASSERT_SENSOR_RANGE` is a macro specifically so that `__FILE__` and `__LINE__` report the call site, not the macro's own definition. Suppose you rewrote it as an ordinary function `void assert_sensor_range(float val, float min, float max)` and called it from three different places in `main.c`. What would `__FILE__` and `__LINE__` print from inside that function, and why would all three calls report the same thing?

**Challenge 2 — Additive**
`engine.c`'s `thruster_bit()` clamps any thruster number `>= 4` to bit 0, because `ENGINE_CTRL`'s thruster field is 4 bits wide — but the `4` appears as a bare literal in that comparison. Add `#define THRUSTER_COUNT 4` to `engine.h` and replace the literal `4u` in `thruster_bit()` with `THRUSTER_COUNT`.

**Challenge 3 — Additive**
The boot banner in `main.c` always prints `BOOT_CONFIG`'s raw bytes as hex — useful for development, not something a production boot sequence needs to show. Wrap that `for` loop and its surrounding `printf` calls in `#ifdef DEBUG_TELEMETRY` / `#endif`, the same pattern used for the periodic scan's debug line. Build once with the flag and once without, and confirm the boot banner differs.

**Challenge 4 — Analytical**
"What this phase revealed" points out that `ASSERT_SENSOR_RANGE` evaluates `val` twice in its expansion, which would silently double-call a function passed as the argument. The C standard library's own `assert(expr)` macro also references `expr` more than once internally (once to test it, once to print it on failure) — yet `assert` is considered safe to use with arbitrary expressions in practice. What convention do C programmers follow when calling macros like `assert` or `ASSERT_SENSOR_RANGE` that avoids ever triggering the double-evaluation hazard, given that the macro itself cannot enforce it?

**Challenge 5 — Additive (stretch)**
Add a second function-like macro, `CLAMP(val, lo, hi)`, that expands to an expression returning `val` clamped into the inclusive range `[lo, hi]` — for example `(((val) < (lo)) ? (lo) : (((val) > (hi)) ? (hi) : (val)))`. Use it inside `engine_set_throttle()` in `engine.c` to clamp `level` to `0`–`15` before writing it into the register, so a caller passing `engine_set_throttle(20)` is silently capped at the register's maximum representable throttle value instead of corrupting adjacent bits.

---

## 💭 Thought pieces for the next branch

1. All telemetry disappears the moment Calypso reboots — there is no persistent record of anything that happened during the previous run. If an anomaly occurred right before a reboot, how would you find out about it afterward, with nothing but what's currently in this program?
2. Every output in this program so far has gone to `printf` and `stdout` — the terminal. What if you wanted that same output to go to a file as well, or instead? What would C need to give you to write to something other than the screen?
3. A float sensor value like `32.7` could be stored as the text `"32.7\n"` or as its raw 4-byte IEEE-754 binary representation. Which is more compact on disk? Which is easier to read back correctly on a different machine? When would you choose each?

---

*Previous branch: [`phase-14_embedded-patterns`]*
*Next branch: [`phase-16_file-io`]*
