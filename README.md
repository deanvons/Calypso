# Phase README — Diagnosing the Computer

> **Phase 02 — Debugging C Programs** | Calypso · Core C

The flight computer's fuel sensor is misreporting values — two bugs are hiding in sensor calculation code that compiles cleanly, runs without crashing, and gives a confident wrong answer.

Before Calypso can fly, its sensor readings must be trustworthy. This phase introduces the two most important tools for finding bugs that the compiler cannot catch: printf-trace debugging and the VS Code interactive debugger. A deliberately-broken fuel sensor simulation gives you a real target to investigate — the wrong output is visible, the cause is not. The lesson is the investigation process itself; the bugs are left unfixed because fixing them requires knowledge of integer types that Phase 3 will introduce.

> **A note on scope.** This phase is about finding bugs, not fixing them. The fuel sensor simulation contains deliberate defects that will be corrected in Phase 3. Resist the temptation to fix the code here — the goal is to practise systematic investigation, not to patch the symptom.

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
| `📌 phase-02_debugging` | **`printf` tracing · VS Code debugger · three C error categories** | — |
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

### Challenge 1 — `%c` vs `%d` on the same `char`

`printf("%c", cmd)` prints the character whose ASCII code equals `cmd` (e.g. `A` for the value 65). `printf("%d", cmd)` prints the same bytes interpreted as a decimal integer (e.g. `65`). Both compile without error because `printf` accepts a variadic argument list and applies no type checking — the format specifier is the only instruction it has about how to interpret the bytes it receives. A `char` is a small integer; `%c` and `%d` are just two different lenses over the same raw value.

### Challenge 2 — Signalling a fault via return value

Return any non-zero value from `main()` to signal failure — the C standard defines `EXIT_FAILURE` (typically `1`) in `<stdlib.h>` for this purpose. The OS passes the return value to whatever launched the program. On Linux and macOS an operator script checks `$?` immediately after the program exits; on Windows it reads `%ERRORLEVEL%`. A mission control script could check for a non-zero exit code, log the anomaly, and suppress a launch authorisation — all without any additional messaging.

### Challenge 4 — Compile-time date vs runtime date

`__DATE__` is substituted by the preprocessor before compilation — a string literal baked into the binary, not computed at runtime. If Calypso runs for months without recompilation, the banner always shows the build date, not the launch date. The runtime alternative is `time()` from `<time.h>`, which returns the current Unix timestamp, combined with `strftime()` to format it as a human-readable string. The distinction between compile-time and runtime values is the key idea here; it resurfaces properly in Phase 15.

### Thought piece 1 — Finding a fault in static output without error messages

Without error messages, the only tool available is systematic tracing — inserting `printf` calls at key points to print the value of every variable that might be wrong, then narrowing down which one diverges from the expected value first. That is printf-trace debugging. The VS Code debugger automates the same process: instead of modifying source to add prints, you set breakpoints and inspect values interactively. Phase 2 covers both techniques directly.

### Thought piece 2 — `scanf` and a non-matching format specifier

For `%c`, any character is a valid match — no mismatch is possible. For numeric specifiers like `%d`, typing letters leaves the input unconsumed in the buffer and `scanf` returns `0` (number of successful conversions). Without checking the return value, the program continues with whatever value the variable held before the call — possibly uninitialised, possibly stale. No crash, no error message: silently wrong behaviour. Checking `scanf`'s return value is the correct guard.

### Thought piece 3 — Linker error vs compiler error

A linker error means the source compiled successfully but the linker could not find the machine code for a symbol the object file references. The bug is not in the source text but in what gets linked: a missing source file, a missing `-l` flag, or a function declared but never defined. The compiler accepts the declaration; the linker fails when it cannot find the body.

---

## 💡 Why we made this decision

### Introduce deliberate bugs rather than asking you to imagine them

If you have only ever written working code, you have no practice investigating misbehaving code. The natural impulse is to stare at the source until the bug becomes obvious — which works when the codebase is ten lines long and fails completely when it is not. The only way to develop systematic debugging instincts is to practise them on real misbehaving code. A deliberately-broken function here, before the codebase grows complex, means you have these investigation skills when you genuinely need them in later phases.

The two bugs chosen — an off-by-one and a sign error — are the most common classes of logical error in numerical code. The sign error (adding consumed instead of subtracting it) is the kind of bug that often slips through code review because the arithmetic looks plausible at a glance. The off-by-one (inflating the burn period by one extra cycle) is subtle enough that checking the logic step-by-step you might miss it on the first pass. Together they give you a realistic experience of debugging that requires more than visual inspection.

### Leave the bugs unfixed

Fixing the fuel sensor simulation here would blur two separate lessons. Phase 2 is about finding bugs — the investigation process. Phase 3 is about choosing the right integer types so that certain classes of bug become impossible. If we fix the bugs here, you lose the concrete motivation for Phase 3: you would not feel the problem that `int`-everywhere creates. The unfixed bugs are not an oversight — they are the setup for the next phase's question: "would the right type have prevented this?"

### `printf` tracing before the interactive debugger

Both techniques are introduced in this phase, but `printf` tracing comes first for two reasons. First, it requires only what you already know — `printf` from Phase 1. Second, it is the technique that works everywhere: on desktop programs, inside embedded firmware, in situations where no interactive debugger is available. The VS Code debugger is more powerful and more efficient for desktop code, but if you understand printf tracing first you will never be helpless in an environment where the debugger is not an option.

---

## ⏮️ What we built in the previous branch

`phase-01_boot-and-io` gave Calypso a voice: a boot banner printed with `printf` using format specifiers, and a command prompt that reads a single character from the operator with `scanf`. The program is a single `main.c`, compiles with one GCC invocation, and exits after one round of I/O. It proves the toolchain works and gives the shuttle an identity, but it cannot compute anything or detect any error in its own output.

---

## 🎯 What we're doing in this branch

- Add the fuel sensor simulation to `main.c` — code that should compute remaining fuel level but contains an off-by-one and a sign error that cause it to misreport
- Call the function from `main()` and print both the sensor reading and the expected value so the discrepancy is immediately visible
- Walk through `printf`-trace debugging in the README: adding intermediate `printf` calls inside the function to expose where the wrong value first appears
- Walk through the VS Code debugger in the README: setting a breakpoint inside the function, stepping line by line, and inspecting the call stack and variable values

---

## 🧑🏻‍🏫 Learning goals

### Understand
- **Identify** the three categories of C errors — syntax errors, logical errors, and undefined behaviour — and explain why undefined behaviour is the most dangerous of the three
- **Contrast** `printf`-based debugging with interactive debugger use: when each is the right tool, and what trade-offs matter in desktop vs embedded development contexts

### Apply
- **Use** `printf`-based tracing to inspect intermediate values in the sensor calculation and confirm which code paths execute at runtime
- **Set** breakpoints in Visual Studio Code and step through execution line by line to observe program state at any point during a run
- **Inspect** variable values and the call stack in the Visual Studio Code debugger to locate where the actual fuel reading diverges from the expected value
- **Create** a conditional breakpoint and explain when it is more efficient than an unconditional one

---

## 🔑 Key concepts

| Concept | Plain English |
|---|---|
| **Syntax error** | A mistake the compiler catches before producing any output — invalid C grammar (missing semicolon, mismatched braces). The program cannot compile. |
| **Logical error** | The program compiles and runs without crashing, but produces the wrong output. The compiler has no way to detect it; only systematic testing or tracing reveals it. |
| **Undefined behaviour** | A situation the C standard makes no promise about — the program may crash, produce garbage output, or appear to work correctly depending on the platform, compiler version, and phase of the moon. It is more dangerous than a logical error because it may be invisible until it causes a failure in production. |
| **`printf` tracing** | Temporarily inserting `printf` calls at key points in the code to print variable values and confirm which paths execute. Works anywhere C runs; no tools required. |
| **Breakpoint** | A marker on a source line that tells the debugger to pause execution when it reaches that line, letting you inspect the program's state before it continues. |
| **Call stack** | The ordered list of active code blocks that got execution to its current point — each frame shows where control came from and what values were in scope at that moment. |
| **Conditional breakpoint** | A breakpoint that only triggers when a specified Boolean condition is true. Useful when a bug only manifests after many calls or when a particular value is reached. |
| **Step over / step into** | Two debugger navigation commands. Step over executes the current line and moves to the next without descending into any function it calls. Step into descends into the called function so you can trace its execution line by line. |

---

## 🔍 What to notice in the code

**[`main.c` — the sensor simulation block](main.c)**
The `simulate_fuel_sensor` block sits above `main()`. It has two `DELIBERATE` comments marking the exact lines that contain the bugs, but do not read them yet — try to locate the bugs through tracing first. The block takes three `int` values and returns one. The inputs and the return value are all you can see from `main()`.

**[`main.c` — the discrepancy output in `main()`](main.c)**
The three `printf` calls after the sensor calculation print the raw reading, the arithmetic result you would expect if the calculation were correct, and the difference between them. The discrepancy of 105 kg — with a tank capacity of 1000 kg — is the signal that something is wrong. The trace you add in Challenge 3 goes in `main()` around these same lines, printing the inputs before the calculation and `fuel_reading` after it.

**How to add a printf trace (for Challenge 3)**
Add a `printf` before the sensor calculation to print `initial_fuel`, `burn_rate_ks`, and `elapsed`:
```c
printf("DEBUG: inputs -- initial=%d  burn_rate=%d  elapsed=%d\n", initial_fuel, burn_rate_ks, elapsed);
```
Then look at what comes back in `fuel_reading`. The inputs are correct — the bug is in the calculation itself, not in how `main()` calls it. The trace from `main()` can tell you that; it cannot tell you which intermediate value inside the calculation is wrong first. That is where the debugger takes over.

**How to use the VS Code debugger (for Challenges 4 and 5)**
Open `main.c` in VS Code. Click in the left gutter next to the first line of the `simulate_fuel_sensor` block to set a breakpoint. Press `F5` to start a debug session (select "C/C++: cl.exe build and debug active file" if prompted, or use the existing launch configuration). Execution pauses at the breakpoint. Use `F10` (step over) to advance one line at a time and watch the **Variables** panel update. Use the **Call Stack** panel to see how execution arrived at the current line. To set a conditional breakpoint: right-click the breakpoint dot and choose "Edit Breakpoint", then enter a condition expression such as `consumed > initial_level`.

---

## 🔗 What this phase revealed

The bugs in the fuel sensor simulation use `int` for every value — initial level, burn rate, elapsed time, consumed fuel. `int` is platform-dependent in width: on a 16-bit MCU it may be 2 bytes, giving a maximum value of 32,767. A fuel reading that exceeds that limit wraps around silently. The sign error that causes fuel to increase rather than decrease is a logical error; the overflow that would occur if we tried to represent a realistic fuel mass in a 16-bit `int` is a category of silent data corruption that the current type selection cannot prevent.

> **LEARNING MOMENT:** Using `int` for sensor values is not a neutral default — it is a choice with consequences. The right type for a sensor value depends on the platform, the value range, and what must happen when the value exceeds that range. That is exactly what Phase 3 introduces.

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
gcc main.c -o calypso
./calypso
```

The program prints the boot banner, runs the sensor suite check, shows the discrepancy, then prompts for a command character and a crew ID. Press any key followed by Enter at each prompt.

**Expected output (sensor section):**
```
--- Sensor Status ---
Fuel sensor reading  : 1055 kg
Expected fuel level  : 950 kg
Discrepancy          : 105 kg

WARNING: fuel sensor mismatch detected. Investigate before launch.
```

**For VS Code debugging:** Open the repo folder in VS Code with the C/C++ extension installed. Set a breakpoint by clicking in the gutter next to any line, then press `F5` to launch the debugger.

---

## ✏️ Challenges for students

**Challenge 1 — Analytical**
The simulation reports a fuel level higher than the full tank and rising. Before reading the implementation, classify this as a syntax error, a logical error, or undefined behaviour — and explain your reasoning. What does your classification tell you about the right investigation strategy?

**Challenge 2 — Analytical**
You have two tools: `printf` tracing and the VS Code debugger. For each of the following scenarios, identify which tool you would reach for first and explain why: (a) a calculation that runs 10,000 times and only produces the wrong result on iteration 9,347; (b) firmware running on a bare-metal MCU with no OS and no debugger port available.

**Challenge 3 — Additive**
Add `printf` calls to `main()` — before the sensor calculation to print the three input values (`initial_fuel`, `burn_rate`, `elapsed`), and after it to print `fuel_reading` alongside the expected result. Run the program and use that output to explain what the trace confirms: are the inputs correct, and does the output match what correct arithmetic would produce? What can you conclude from `main()` alone, and what can you not yet determine?

**Challenge 4 — Analytical**
In the VS Code debugger, set a breakpoint on the first line of the sensor calculation and step through to the last line. Open the call stack panel. What information does it show you that the `printf` trace from Challenge 3 did not? Name one debugging question that the call stack answers that `printf` tracing cannot answer efficiently.

**Challenge 5 — Additive (stretch)**
Set a conditional breakpoint on the last line of the sensor calculation that only triggers when the result is greater than `initial_level` (which it always is, given the sign error). What condition expression did you enter in VS Code? Explain why this is more efficient than an unconditional breakpoint when the same sensor read is performed many times in sequence — for example, once per second over a 10-minute mission.

---

## 💭 Thought pieces for the next branch

1. The buggy function uses `int` for sensor values. Is `int` always the same size on every platform? What breaks if we depend on that assumption on a 16-bit MCU?
2. When `printf` sees `%d`, how does it know how many bytes to read from the argument? What would happen if the format specifier and the actual type did not match in size?
3. The off-by-one caused a value to inflate rather than wrap around — but on real embedded hardware, what are the consequences of silent integer overflow in a fuel sensor reading?

---

*Previous branch: [`phase-01_boot-and-io`]*
*Next branch: [`phase-03_integer-types`]*
