# Guide 2 - Real-Time Systems (Raspberry Pi + GPIO)

Guide to practical exercises in C oriented to real-time events using **Raspberry Pi** and the **pigpio** library.

## Content

- `blink_led.c`: periodic blinking of an LED with timing in microseconds.
- `interrupted_button.c`: LED control by button interrupt/callback with anti-bounce.
- `jitter.c`: Latency measurement and jitter calculation in button events.

## Requirements

- Raspberry Pi (or GPIO-compatible environment).
- Library `pigpio` installed.
- GCC Compiler (recommended `gcc-12`).

Typical installation on Debian/Raspberry Pi OS:

```bash
sudo apt update
sudo apt install -y pigpio libpigpio-dev gcc-12
```

## Compilation

From folder `Guide 2`:

```bash
gcc-12 blink_led.c -o blink_led -lpigpio -lrt -pthread
gcc-12 interrupted_button.c -o interrupted_button -lpigpio -lrt -pthread
gcc-12 jitter.c -o jitter -lpigpio -lrt -pthread
```

## Execution

Since `pigpio` accesses hardware, it normally runs with elevated privileges:

```bash
sudo ./blink_led
sudo ./interrupted_button
sudo ./jitter
```

To stop programs in a loop: `Ctrl + C`.

## Expected connections

- LED in `GPIO 4` (`LED_GPIO`).
- Button in `GPIO 17` (`BUTTON_GPIO`) for `interrupted_button.c` y `jitter.c`.
- Code uses intern `pull-up` (`PI_PUD_UP`):
	- loose button -> logic level `1`
	- pressed button -> logic level `0`

## What each exercise does

### 1) `blink_led.c`

- Alternates the LED every `500000 us` (500 ms).
- Use `gpioTick()` to maintain a constant period and reduce temporal drift.
- Use `SIGINT` to terminate and release resources.

### 2) `interrupted_button.c`

- Register callback with `gpioSetAlertFunc(...)`.
- Turns LED on/off according to button status.
- Apply bounce filter with `gpioSetGlitchFilter(BUTTON_GPIO, 10000)`.

### 3) `jitter.c`

- Capture up to `N = 20` latency measurements (`button event -> LED on`).
- Calculate and display:
	- initial latency,
	- minimum latency,
	- maximum latency,
	- average latency,
	- jitter (`max - min`).

## Notes

- If no hardware is connected, programs that rely on GPIO will not function correctly.
- Avoiding `printf` within critical callbacks improves the accuracy of latency measurements.