# Air Conditioning Controller (Guide 6)

This guide contains a concurrent real-time style application in `air_conditioning.c`.
The program reads temperature from an **AHT10** sensor over **I2C**, applies a finite state machine (FSM), and controls a fan connected to Raspberry Pi GPIO.

## Overview

The system runs three threads with different responsibilities:

- **Thread A (`taskA`)**: reads temperature from AHT10 via I2C and updates a shared variable.
- **Thread B (`taskB`)**: executes the FSM (`REPOSE`, `ALERT`, `VENTILATION`) and turns the fan ON/OFF.
- **Thread C (`taskC`)**: prints diagnostics (temperature + state) periodically.

A shared mutex (`pthread_mutex_t`) protects shared data (`global_temp`, `global_state`).

## State Machine Logic

The controller behavior is:

- **REPOSE**
  - Fan OFF.
  - If temperature `> 30.0 C`, transition to `ALERT` and start alert timer.

- **ALERT**
  - Fan still OFF.
  - If temperature `<= 30.0 C`, return to `REPOSE`.
  - If temperature remains high for `>= 60 s`, transition to `VENTILATION` and turn fan ON.

- **VENTILATION**
  - Fan ON.
  - If temperature `< 25.0 C`, fan OFF and return to `REPOSE`.
  - If ventilation lasts `>= 120 s`, fan OFF and return to `REPOSE`.

Timing for alert/ventilation windows is tracked with `gpioTick()`.

## Requirements

- Raspberry Pi (or compatible board with GPIO + I2C support).
- AHT10 temperature sensor.
- Fan/actuator controlled from a GPIO output (through proper driver/transistor/relay).
- `pigpio` library and development headers.
- GCC compiler.

Typical install on Raspberry Pi OS / Debian:

```bash
sudo apt update
sudo apt install -y pigpio libpigpio-dev build-essential
```

## Hardware Mapping

- **AHT10 (I2C)**
  - I2C bus: `1`
  - I2C address: `0x38`

- **Fan output**
  - GPIO pin: `17` (`FAN_PIN`)

## Enable I2C

If I2C is not already enabled:

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

Then reboot if requested.

## Build

From `Guide 6`:

```bash
gcc air_conditioning.c -o air_conditioning -lpigpio -lpthread -lrt
```

## Run

```bash
sudo ./air_conditioning
```

`sudo` is generally required for GPIO access and real-time scheduling settings.

Stop with `Ctrl + C` (SIGINT). The program exits gracefully and releases resources.

## Runtime Output

Thread C prints the current condition every 5 seconds:

- Current temperature (C)
- Current state (`REPOSE`, `ALERT`, `VENTILATION`)

Example:

```text
=== SYSTEM STATUS ===
Temp: 31.45 C
State: ALERT
=====================
```

## Notes and Troubleshooting

- If thread creation fails with scheduling-related errors, run with `sudo`.
- The code uses `SCHED_FIFO` for control threads; this may require real-time privileges.
- If I2C communication fails (`Error opening I2C` / `Error sending data of AHT10`), verify:
  - physical wiring,
  - sensor power,
  - I2C enabled,
  - sensor address with `i2cdetect -y 1`.
- For real hardware, never drive a fan directly from GPIO pin current. Use an appropriate driver stage.
