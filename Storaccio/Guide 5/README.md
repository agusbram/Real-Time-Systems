# Concurrent Flight Control Simulator (Guide 5)

This guide contains a concurrent real-time simulation in `concurrent_flight.c`.
The program models three flight-control tasks running in parallel with POSIX threads and Linux scheduling policies.

## Overview

The simulator runs three worker threads:

- **Stability Control** (critical task, highest priority in RT mode)
- **Navigation** (medium-priority task)
- **Telemetry** (low-priority periodic task)

It compares behavior across three scheduling scenarios:

- **Case A - Normal**: `SCHED_OTHER` (default Linux fair scheduler)
- **Case B - Real-Time**: `SCHED_FIFO` (strict fixed-priority scheduling)
- **Case C - Priority Inversion**: `SCHED_FIFO` + shared mutex contention

The experiment runs for **10 seconds** and prints a final comparative table with iteration counts and relative CPU share.

## Key Concepts Demonstrated

- POSIX threads (`pthread`)
- Linux scheduling policies (`SCHED_OTHER`, `SCHED_FIFO`)
- Fixed-priority preemption behavior
- Priority inversion with shared resources
- Periodic activation using POSIX timer (`timer_create`, `timer_settime`)
- Signal-driven periodic thread wake-up (`sigwaitinfo` + `SIGRTMIN`)

## Build

From the `Guide 5` folder:

```bash
gcc -Wall -O0 concurrent_flight.c -o concurrent_flight -lpthread -lrt -lm
```

## Run

```bash
./concurrent_flight A
sudo ./concurrent_flight B
sudo ./concurrent_flight C
```

`sudo` is required for real-time scheduling (`SCHED_FIFO`) in cases B and C.

## Cases and Expected Behavior

### Case A - Normal (`A`)

- Uses `SCHED_OTHER`.
- Linux CFS tries to distribute CPU time fairly.
- Iteration counts are usually closer to each other.

### Case B - Real-Time (`B`)

- Uses `SCHED_FIFO` with priorities: Stability `80`, Navigation `40`, Telemetry `10`.
- Highest-priority runnable thread dominates CPU execution.
- Stability should complete significantly more iterations than the others.

### Case C - Priority Inversion (`C`)

- Uses `SCHED_FIFO` plus a shared mutex.
- Low-priority telemetry thread can hold the mutex while high-priority stability waits.
- Medium-priority navigation may run while stability is blocked, demonstrating priority inversion.

## Program Output

During execution, the program logs:

- scheduler setup per thread
- telemetry packets every 500 ms
- lock/unlock messages during inversion demo (case C)

At the end, it prints a results table with:

- iterations per thread
- configured priority
- percentage contribution relative to total iterations
- analysis notes for the selected case

## Notes

- The telemetry task is activated by a **POSIX interval timer** every 500 ms.
- The timer signal is delivered specifically to the telemetry thread (Linux-specific `SIGEV_THREAD_ID`).
- If real-time policy setup fails, warnings are shown and you should run with `sudo`.

## Troubleshooting

- **`Unknown case` error**: use only `A`, `B`, or `C`.
- **Real-time scheduling warnings**: rerun with `sudo`.
- **Linker math errors** (e.g., undefined reference to `sin`/`cos`/`sqrt`): ensure `-lm` is included.
