# Gyroscope/Accelerometer Real-Time Pipeline (Guide 7)

This guide documents the program in `giroscope.c`.
The application reads acceleration data from an **MPU6050** over **I2C**, filters it, prints CSV output, and optionally drives a servo motor from the filtered X-axis signal.

## What The Program Does

The system is built as a producer-consumer pipeline with an additional actuator task:

- **Thread A (`taskA`) - Acquisition (Producer)**
  - Reads raw accelerometer data (`X`, `Y`, `Z`) at **100 Hz** (every 10 ms).
  - Sends each sample to a POSIX message queue (`/imu_queue`).

- **Thread B (`taskB`) - Processing (Consumer)**
  - Receives samples from the queue.
  - Converts raw values to `g` units using `16384 LSB/g` (`+/-2g` mode).
  - Applies a moving-average filter (`N = 10`).
  - Prints filtered output to stdout in CSV format: `fx,fy,fz`.
  - Updates a shared X-based angle value for the actuator task.

- **Thread C (`taskC`) - Servo Actuation (Optional)**
  - Reads the shared filtered angle.
  - Clamps it to `[-90, 90]` degrees.
  - Maps that angle to servo pulses (`500` to `2500` microseconds).
  - Commands a servo on GPIO 18 using `gpioServo()`.

Shared variables are protected with `pthread_mutex_t`.

## Scheduling And Concurrency

- `taskA`: `SCHED_FIFO`, priority `80`
- `taskB`: `SCHED_FIFO`, priority `60`
- `taskC`: `SCHED_OTHER`, priority `0`

Thread communication uses a non-blocking POSIX queue:

- max pending messages: `10`
- message size: `sizeof(ImuData)`

If the queue is full, the producer drops the current sample (`EAGAIN`) to keep timing stable.

## Requirements

- Raspberry Pi (or compatible board with GPIO + I2C)
- MPU6050 sensor
- Servo motor (for `taskC`, optional)
- `pigpio` runtime and development headers
- GCC

Install dependencies (Raspberry Pi OS / Debian):

```bash
sudo apt update
sudo apt install -y pigpio libpigpio-dev build-essential
```

## Hardware Mapping

- **MPU6050**
  - I2C bus: `1`
  - I2C address: `0x68`
  - First data register read: `ACCEL_XOUT_H (0x3B)`

- **Servo**
  - GPIO: `18` (`MOTOR_GPIO`)

## Enable I2C

If I2C is disabled:

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

Reboot if requested.

## Build

From `Guide 7`:

```bash
gcc giroscope.c -o giroscope -lpigpio -lpthread -lrt
```

## Run

```bash
sudo ./giroscope
```

`sudo` is typically needed for GPIO access and real-time scheduling policies.

Stop with `Ctrl + C`. The SIGINT handler stops all loops cleanly, then the program joins threads and releases queue/GPIO resources.

## Run With The Python Plotter (Linux Pipe)

The C program and Python plotter can run together through a Linux pipe.
From `Guide 7`:

```bash
gcc giroscope.c -o giroscope -lpigpio -lpthread -lrt
sudo ./giroscope | python3 3axis_plotter.py
```

In this setup:

- `./giroscope` writes filtered CSV samples to stdout.
- `3axis_plotter.py` reads those samples from stdin and plots the 3 axes in real time.

Use `Ctrl + C` to stop both processes.

## Output

The program prints one filtered sample per line:

```text
fx,fy,fz
```

Example:

```text
0.0123,-0.0345,0.9981
0.0101,-0.0302,0.9990
```

This output is designed to be piped into plotting/logging scripts (for example, `3axis_plotter.py`).

## MPU6050 Configuration Used

- `PWR_MGMT_1 (0x6B) = 0x00` (wake-up)
- `ACCEL_CONFIG (0x1C) = 0x00` (`+/-2g` full scale)
- 6 bytes read from `ACCEL_XOUT_H (0x3B)`

## Troubleshooting

- If I2C open/read fails:
  - verify wiring and power,
  - ensure I2C is enabled,
  - check the sensor with `i2cdetect -y 1`.
- If scheduling or thread creation fails, run with `sudo` and verify real-time privileges.
- Servo jitter can happen with poor power supply; use an adequate external source and common ground.
- Dropped samples under queue pressure are expected behavior by design.
