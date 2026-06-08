## Product Requirement Document

### What it is

Calypso is a command-line flight computer simulation for an interplanetary crew shuttle. An operator interacts via terminal: querying sensor readings, advancing mission phases, managing crew records, and issuing engine control commands. The computer enforces mission constraints — fuel limits, temperature thresholds, crew capacity — and writes a continuous flight log to disk. The finished system is a multi-file C program that compiles cleanly, runs end-to-end through a simulated mission from pre-launch to docked, and produces a persistent `calypso.log` on exit.

### Core use cases

1. A user can boot the flight computer and see a system status report so that they can confirm all sensors are online before launch.
2. A user can issue an engine control command (enable/disable thruster pair, set throttle level) so that the shuttle can manoeuvre between mission phases.
3. A user can query the crew manifest — name, rank, and assignment — so that the operator knows who is aboard at any point in the mission.
4. A user can advance the mission phase (`PREFLIGHT → LAUNCH → CRUISE → APPROACH → DOCKED`) so that the state machine enforces the correct sequence of operations.
5. A user can review the flight log file after the mission so that any anomalies are permanently recorded regardless of system reboot.

### Out of scope

- Networking, multi-threading, or signal handling
- Graphics or TUI libraries — standard terminal I/O only
- Accurate physics simulation — sensor values are plausible, not real
- Platform-specific APIs; must compile on Linux, macOS, and Windows with GCC or Clang