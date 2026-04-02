# Barrier System (Guide 4)

This guide contains a real-time barrier control example implemented in `barrier_system.c` for Raspberry Pi using `pigpio`, POSIX threads, and a POSIX timer.

## Overview

The program simulates an automatic barrier with three concurrent contexts running at the same time:

- Main thread: moves the servo continuously between 0 and 180 degrees.
- Button monitoring thread (`SCHED_FIFO`, max priority): watches an emergency button.
- Timer callback (`report()` every 1 second): prints system state and current servo position.

When the emergency button is pressed, the system enters alert mode:

- Servo movement pauses immediately.
- Alert LED turns on.
- A message is printed to the console.

When the button is released, the system returns to normal mode:

- Alert LED turns off.
- Servo movement resumes.

## Hardware Mapping

- `GPIO 23` -> Alert LED (output)
- `GPIO 17` -> Emergency button (input, pull-down + glitch filter)
- `GPIO 18` -> Servo motor PWM control (output)

## Main Features

- Real-time style scheduling for the button thread with `SCHED_FIFO`.
- Shared state protected with `pthread_mutex_t`.
- Graceful shutdown with `SIGINT` (`Ctrl+C`).
- Periodic monitoring report with POSIX timer (`timer_create`, `timer_settime`).
- Debounced button input using `gpioGlitchFilter()`.

## Build

Compile on Raspberry Pi (Linux) with:

```bash
gcc barrier_system.c -o barrier_system -lpigpio -lpthread -lrt
```

If your system requires additional flags or different library paths for `pigpio`, adjust accordingly.

## Run

```bash
sudo ./barrier_system
```

`sudo` is typically required for GPIO access.

## Runtime Behavior

- At startup, the servo is initialized at 0 degrees (500 us pulse width).
- In normal mode, the servo sweeps up and down smoothly.
- Every second, a report is printed:
	- `[REPORT] Position: X | State: NORMAL`
	- `[REPORT] Position: X | State: ALERT`
- On button press, the program prints:
	- `[ALERT] Emergency stop activated - Latency detected`
- Press `Ctrl+C` to stop the program cleanly.

## Notes

- The servo pulse width is mapped linearly from angle to microseconds:
	- `0 deg -> 500 us`
	- `180 deg -> 2500 us`
- The emergency flag has priority over motion updates.
- The button thread is designed to preempt other work quickly due to its high scheduling priority.
