# Phase README — Sensor History Buffer

> **Phase 09 — Arrays** | Calypso · Core C

Fixed-size circular history buffers for two sensor channels — filling each slot with a `for` loop, computing averages, and detecting drift across a sequence of readings.

Every sensor in the current system returns a single snapshot — one `uint16_t` reading per call. That is sufficient for a dashboard display, but it tells you nothing about whether the value has been rising, falling, or spiking over the last several seconds. Anomaly detection — the kind that catches a slow fuel leak before it becomes a crisis — requires a sequence. The fix is to store readings over time, which means allocating a block of memory large enough to hold N values of the same type.

An array is the simplest possible answer: a fixed-size, contiguous block of same-type elements, each addressable by a zero-based index. `uint16_t fuel_history[SENSOR_HISTORY_LEN]` reserves ten consecutive `uint16_t` slots in memory. A `for` loop fills them. `sizeof(fuel_history) / sizeof(fuel_history[0])` derives the element count without repeating the number. And when the buffer is passed to `compute_average()`, C does not copy all twenty bytes — it converts the array name to a pointer to its first element. That conversion, and what it means for `sizeof` inside the function, is the hidden lesson of this phase.

> **A note on scope.** Arrays in C decay to a pointer to their first element when passed to a function — this is how `compute_average()` can read the buffer without copying it. Pointers are the subject of Phase 10 and are introduced here only to explain the decay. Writing back through a pointer, pointer arithmetic as a standalone tool, and multi-level pointers are Phase 10 material.

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
| `📌 phase-09_arrays` | **Sensor history buffers · `sizeof` element count · `array[i]` ≡ `*(array + i)`** | — |
| `phase-10_pointers` | `&` / `*` · pointer arithmetic · `const T*` vs `T* const` · `**` | — |
| `phase-11_strings` | `char` arrays · `strncpy` / `strcmp` / `strlen` · null terminator | — |
| `phase-12_structs` | `crew_member_t` · `spacecraft_t` · dot / arrow notation · nested structs | — |
| `phase-13_dynamic-memory` | `malloc` / `realloc` / `free` · dynamic crew roster · `NULL` checks | — |
| `phase-14_embedded-patterns` | `volatile` · memory-mapped I/O pointer · struct bitfields · `const` ROM data | Hardware abstraction |
| `phase-15_preprocessor` | `#define` constants · include guards · `#ifdef DEBUG_TELEMETRY` · function-like macro | — |
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

### Challenge 1 — What C copies in pass-by-value

When you call `sensors_apply_calibration(fuel, 5)`, C evaluates `fuel` at the call site and copies that integer value into the function's `reading` parameter. `reading` is a new, separate variable on the stack — it holds the same number, but it is not the same storage location. There is no path back to the caller's `fuel` from inside the function because the function was never given the address of `fuel`, only its value. The returned result is a second copy, computed from the modified local and handed back through the return register.

### Challenge 2 — `static` file scope and what it prevents

`static uint32_t ENGINE_CTRL` at file scope in `engine.c` limits the variable's visibility to the translation unit it is declared in. No other `.c` file can name it — there is no declaration visible outside `engine.c`, so any attempt to write `ENGINE_CTRL = ...` in `main.c` is a compile error. If you replaced `static` with no qualifier and added `extern uint32_t ENGINE_CTRL` in `engine.h`, every file that includes `engine.h` could read and write the register directly. Two specific bugs that `static` makes impossible: the navigation section accidentally clearing `ENGINE_CTRL` with a stray assignment, and the state machine in `main.c` writing a bit position directly instead of calling the function that validates the throttle range.

### Challenge 3 — Naming convention and the `printf` family

Following the module prefix convention in this codebase, the function would be named `sensors_read_pressure()`. The C standard library solves the same naming problem differently: the `printf` family uses suffixes that encode the output target — `printf` writes to `stdout`, `fprintf` to a `FILE *`, `sprintf` to a `char` buffer, `snprintf` adds a length bound. Each is a distinct function with a distinct name; C has no overloading to let them share one. The convention in both cases is the same: the name encodes what the function operates on or where its output goes.

### Thought piece 1 — A data structure that holds a sequence of values

An array. `uint16_t fuel_history[SENSOR_HISTORY_LEN]` reserves ten consecutive `uint16_t` slots in memory — each addressable by index from 0 to `SENSOR_HISTORY_LEN - 1`. That is exactly the structure `compute_average()` needs: a fixed block of readings, all of the same type, all adjacent, all addressable. This phase adds that structure to Calypso's sensor module.

### Thought piece 2 — Passing an address instead of a copy

A pointer to the variable. If a calibration function needs to write a corrected value back to the caller's variable, it needs the address of that variable — not a copy of its current content. You would pass the address with `&reading` at the call site; the function receives a `uint16_t *` parameter and writes through it with the dereference operator `*`. Phase 10 builds this mechanism in full.

### Thought piece 3 — Memory for a sequence of `uint16_t` values

One `uint16_t` occupies 2 bytes. Twenty of them occupy 40 bytes — they sit contiguously in memory with no gaps between elements of the same type. When you pass an array to a function, C does not copy all 40 bytes; it passes a pointer to the first element. The function receives 8 bytes on a 64-bit platform — the address of the start of the buffer. It can reach any element from there using index arithmetic, but it does not receive the data itself.

---

## 💡 Why we made this decision

### A snapshot is not a trend

`sensors_read_fuel()` returns the current reading. That is sufficient for a dashboard display, but it tells you nothing about whether the value has been rising, falling, or spiking intermittently over the last ten seconds. Catching a slow fuel leak before it becomes a crisis requires a sequence. The fix is to store readings over time in a block of memory large enough to hold N values of the same type and addressable by index.

An array is the simplest possible answer. The alternative — N separate named variables (`reading_0`, `reading_1`, ... `reading_9`) — does not scale and cannot be iterated. An array is what a `for` loop was built for.

### `SENSOR_HISTORY_LEN` as the single source of truth

The buffer length appears in three places: the declaration, the loop that fills it, and the element count passed to analysis functions. All three use `SENSOR_HISTORY_LEN`. If you change one number, every use updates. If the length were a magic literal — `[10]` here, `10` there, `10` again in the loop bound — a change in one place would silently leave the others stale, and the resulting out-of-bounds access would be undefined behaviour with no compile-time warning. `SENSOR_HISTORY_LEN` is a preprocessor `#define` here; Phase 15 covers `#define` properly and explains why this form is preferred over `const int` for array sizes in C.

### Array decay to pointer

When you write `compute_average(fuel_history, SENSOR_HISTORY_LEN)`, you are not copying the entire buffer. C converts `fuel_history` — the array name — to a pointer to its first element and passes that address. The function signature receives `uint16_t *buf` — a memory address, not a copy. This is not a special rule; it is the fundamental connection between arrays and pointers in C: `array[i]` is defined as `*(array + i)`, and the array name in an expression context is a pointer to element zero. One consequence is that `sizeof(buf)` inside `compute_average()` gives the pointer size, not the array size — which is why the buffer length must be passed as a separate parameter. Phase 10 explores this connection from the pointer side.

```mermaid
flowchart TD
    main["main.c\n(records readings on each scan)"]
    record["sensors_record_fuel(reading)\nsensors_record_velocity(reading)\n— fills circular buffer by index —"]
    buffers["static fuel_history[SENSOR_HISTORY_LEN]\nstatic velocity_history[SENSOR_HISTORY_LEN]\n(file-scope, invisible outside sensors.c)"]
    helpers["static compute_average(buf, len)\nstatic detect_drift(buf, len, threshold)\n— receive pointer to first element —"]
    query["sensors_compute_fuel_avg()\nsensors_detect_fuel_drift()\n— public: pass buffer + length to helpers —"]

    main -->|calls| record
    record -->|writes index slot| buffers
    main -->|calls| query
    query -->|passes array + length| helpers
    helpers -->|reads via buf[i]| buffers
```

---

## ⏮️ What we built in the previous branch

`phase-08_functions` split the 490-line `main.c` monolith into four translation units: `sensors.c/sensors.h`, `engine.c/engine.h`, `navigation.c/navigation.h`, and a slimmed-down `main.c` orchestrator. `ENGINE_CTRL` and `ENGINE_STATUS` became `static` file-scope variables in `engine.c`, invisible to all other modules. Pass-by-value was demonstrated explicitly: `sensors_apply_calibration()` modified a local copy of its argument and returned the result; the caller's variable was unchanged. The SOLUTION commit at the top of this branch adds `sensors_read_pressure()` (Challenge 4) and `nav_fuel_efficiency()` (Challenge 5) to their respective modules.

---

## 🎯 What we're doing in this branch

- Declare `static uint16_t fuel_history[SENSOR_HISTORY_LEN]` and `static uint16_t velocity_history[SENSOR_HISTORY_LEN]` in `sensors.c` as file-scope circular buffers; use `SENSOR_HISTORY_LEN` as the single size constant throughout
- Add `sensors_record_fuel()` and `sensors_record_velocity()` to fill each buffer slot by slot using a wrapping index
- Use `sizeof(fuel_history) / sizeof(fuel_history[0])` in the record functions to show how element count is derived from byte size
- Implement static helpers `compute_average(uint16_t *buf, int len)` and `detect_drift(uint16_t *buf, int len, uint16_t threshold)` — the array decays to a pointer at each call site; the helpers demonstrate that `buf[i]` and `*(buf + i)` are identical
- Expose `sensors_compute_fuel_avg()` and `sensors_detect_fuel_drift()` as the public API — these call the helpers, passing the file-scope buffer and its length
- Call the record and query functions from `main.c` in the sensor-scan command so each `s` command adds one reading and prints the running state

---

## 🧑🏻‍🏫 Learning goals

### Understand
- **Explain** what an array is — how elements of the same type are stored contiguously in memory at predictable, fixed-size offsets from the first element
- **Explain** why array indexing starts at 0 and why accessing an index outside the declared bounds is undefined behaviour
- **Explain** why C performs no bounds checking and why this makes out-of-bounds access dangerous — no runtime error, no warning, any result is possible

### Apply
- **Declare** fixed-size arrays in `sensors.c` using the `SENSOR_HISTORY_LEN` constant and access elements using zero-based index notation
- **Use** `sizeof(array) / sizeof(array[0])` to derive element count from the array's byte size
- **Pass** the sensor history buffer to `compute_average()` and `detect_drift()` and explain what the function actually receives

### Analyze
- **Demonstrate** that `buf[i]` and `*(buf + i)` produce the same value — and explain what that equivalence reveals about how array indexing works in memory
- **Examine** what happens to `sizeof` information when an array is passed to a function — and why `sizeof(buf)` inside the function gives the pointer size, not the array size

---

## 🔑 Key concepts

| Concept | Plain English |
|---|---|
| **Array** | A fixed-size block of same-type elements stored contiguously in memory, each addressable by a zero-based integer index. |
| **Zero-based indexing** | The first element is at index 0, the last is at index `length - 1`. Indexing from 0 matches the way pointer arithmetic works: `array[i]` is `*(array + i)`. |
| **Out-of-bounds access** | Reading or writing past the last valid index. C does not check at runtime; the program reads or writes whatever is at that memory address — silent data corruption, a crash, or a security vulnerability with no useful error message. |
| **`sizeof` on an array** | Returns the total byte size of the array — element size multiplied by element count. Dividing by `sizeof(array[0])` recovers the count without repeating the magic number. |
| **Array decay** | When an array name appears in an expression — including as a function argument — C converts it to a pointer to its first element. The function receives an address, not a copy of the data. |
| **`array[i]` ≡ `*(array + i)`** | Array subscripting is defined as pointer arithmetic plus dereference. The compiler generates identical machine code for both forms. |
| **`SENSOR_HISTORY_LEN`** | A preprocessor constant (`#define`) that controls buffer size in one place. Every array declaration, loop bound, and element-count expression references it. Phase 15 covers `#define` in full. |

---

## 🔍 What to notice in the code

**[`sensors.h`](sensors.h)**
Line 9 declares `#define SENSOR_HISTORY_LEN 10` with a NOTE pointing to Phase 15 — this is a preprocessor constant needed for the array size, which C requires to be a compile-time value. Lines 32–35 declare the four history API functions: two record functions that take a reading and two query functions that return results from the internal buffers.

**[`sensors.c:7–10`](sensors.c#L7)**
The two history arrays and their write indices. All four are declared `static` at file scope — they are zero-initialized automatically (C guarantees this for file-scope statics), invisible outside `sensors.c`, and persist for the lifetime of the program. No other file can name `fuel_history` directly; the only access is through the public API.

**[`sensors.c:12–28`](sensors.c#L12)**
`compute_average` is the key teaching function. The block comment explains both facts at once: `sizeof(buf)` inside this function gives the pointer size — not the array size — because the array decayed when it was passed; and `buf[i]` and `*(buf + i)` are identical, because the subscript operator is defined as pointer arithmetic plus dereference. Line 25 shows the equivalence inline in the loop.

**[`sensors.c:72–80`](sensors.c#L72)**
`sensors_record_fuel` uses `sizeof(fuel_history) / sizeof(fuel_history[0])` as the modulo divisor for the circular index wrap. This is the one place in the codebase where `sizeof` is applied to the full array — the array is in scope here, not yet passed to a function, so the compiler knows its total size. Compare this to `compute_average` (line 22), where `sizeof(buf)` gives 8 instead.

**[`sensors.c:87–93`](sensors.c#L87)**
The two public query functions pass `fuel_history` and `SENSOR_HISTORY_LEN` to the static helpers. This is the array decay in action: `fuel_history` in the call expression is a pointer to `fuel_history[0]`; the helpers receive `uint16_t *buf`, not a copy of the 20-byte array.

**[`main.c:229–238`](main.c#L229)**
Inside the `s` command block: each scan records the current fuel and velocity readings into the circular buffers, then prints the running average and drift status. After the first `s`, one slot is filled and nine are zero — the average is low, drift is detected. Press `s` ten times and the buffer fills with real readings; the average stabilises and drift may clear.

---

## ▶️ Running this branch

**Prerequisites:** GCC or Clang (C99+) and CMake 3.10+, or just GCC/Clang on its own.

**With CMake (recommended):**
```bash
cmake -B build
cmake --build build
.\build\Debug\calypso.exe   # Windows (MSVC)
.\build\calypso.exe         # Windows (MinGW)
./build/calypso             # Linux / macOS
```

**Direct compilation (no CMake):**
```bash
gcc -std=c99 main.c sensors.c engine.c navigation.c -o calypso
./calypso
```

The program prints the boot banner, sensor reads, navigation results, and engine demo, then enters the command loop.

| Command | Action |
|---|---|
| `n` | Advance the mission phase (`PREFLIGHT → LAUNCH → CRUISE → APPROACH → DOCKED`) |
| `s` | Run the sensor scan — records fuel and velocity into the history buffers, then prints the running average and drift status |
| `e` | Trigger the emergency shutdown via `goto` |
| `q` | Normal quit via `break` |

**To see the history buffer fill:**
Press `s` repeatedly. After the first press, one slot holds 950 and nine hold zero — the average is 95.0 kg and drift is detected because the real reading deviates far from the zero-padded mean. After ten presses, all slots hold 950, the average stabilises at 950.0 kg, and drift clears.

**Expected output — first sensor scan:**
```
--- Periodic Sensor Scan ---
  Sensor 0:  101.325  [NOMINAL]
  Sensor 1: FAULTED -- skipping
  Sensor 2: WARNING -- reading 3200.000 exceeds HIGH_WARN threshold
ENGINE_STATUS          : 0x00000002
Fuel avg (history)     : 95.0 kg  |  drift: DETECTED
```

**Expected output — tenth sensor scan (buffer full):**
```
Fuel avg (history)     : 950.0 kg  |  drift: none
```

---

## ✏️ Challenges for students

**Challenge 1 — Analytical**
`detect_drift()` receives `uint16_t *buf` and `int len`. Inside the function, `sizeof(buf)` gives 8 on a 64-bit platform — not `SENSOR_HISTORY_LEN * sizeof(uint16_t)`. Explain why. What information is lost when an array is passed to a function, and what must the caller pass explicitly to compensate?

**Challenge 2 — Analytical**
`buf[i]` and `*(buf + i)` produce the same machine code. Explain what the equivalence reveals: what unit does pointer addition operate in, and how does the compiler know the step size for each increment?

**Challenge 3 — Analytical**
`fuel_history` is declared `uint16_t fuel_history[SENSOR_HISTORY_LEN]` at file scope in `sensors.c`. What happens if the loop that fills it writes to index `SENSOR_HISTORY_LEN` — one past the last valid slot? Why doesn't C report an error, and what could the program do next?

**Challenge 4 — Additive**
Add a `static uint16_t pressure_history[SENSOR_HISTORY_LEN]` buffer to `sensors.c`, alongside `sensors_record_pressure(uint16_t reading)` and `sensors_compute_pressure_avg(void)` functions. Follow the same pattern as the fuel buffer. Add the prototypes to `sensors.h`. Call `sensors_record_pressure()` from `main.c` in the sensor-scan command and print the running pressure average alongside the fuel average.

**Challenge 5 — Additive (stretch)**
Modify `sensors_detect_fuel_drift()` so that instead of returning `bool`, it returns the index of the first reading that deviates from the mean by more than the threshold, or `-1` if no such reading exists. Update the return type to `int` in both `sensors.h` and `sensors.c`. Update the call site in `main.c` to print the index when drift is detected rather than a boolean flag.

---

## 💭 Thought pieces for the next branch

1. We pass the sensor buffer into `compute_average()` and it can read the buffer just fine. What if it needed to reset the buffer — write zeros into every slot? It only received a pointer to the first element. Can it write back through that pointer?
2. `array[i]` and `*(array + i)` produce the same result. What does that tell us about how array indexing actually works in memory — what is the compiler computing when it locates element `i`?
3. The calibration function receives a sensor reading and computes a correction — but it operates on a copy. After it returns, the original is unchanged. How do we fix that?

---

*Previous branch: [`phase-08_functions`]*
*Next branch: [`phase-10_pointers`]*
