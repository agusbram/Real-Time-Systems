# Servomotor Control Program

This program controls a servomotor using a button connected to a Raspberry Pi. The servomotor rotates in increments of 30° each time the button is pressed, resetting to 0° after reaching 180°.

## Features

- **Button-controlled rotation**: The servomotor rotates by 30° on each button press.
- **Debounce handling**: Implements a debounce mechanism to avoid false triggers caused by button noise.
- **Finite State Machine (FSM)**: Manages the servomotor's position as states (0°, 30°, ..., 180°).
- **Signal handling**: Gracefully terminates the program with `Ctrl + C`, stopping the servomotor.

## Requirements

- Raspberry Pi (or GPIO-compatible environment).
- **pigpio** library installed.
- GCC Compiler (recommended `gcc-12`).

### Install Dependencies

```bash
sudo apt update
sudo apt install -y pigpio libpigpio-dev gcc-12
```

## Wiring

- **Button**: Connect to `GPIO 17` (`BUTTON_GPIO`).
- **Servomotor**: Connect to `GPIO 18` (`MOTOR_GPIO`).

### Button Logic

- **Pull-down resistor**: The button uses `PI_PUD_DOWN` to ensure stable readings.
  - Button pressed: Logic level `1`.
  - Button released: Logic level `0`.

### Servomotor Logic

- **PWM Signal**:
  - 0°: 500 µs.
  - 180°: 2500 µs.
  - Intermediate positions are calculated proportionally.

## Compilation

From the `Guide 3` folder:

```bash
gcc-12 servomotor.c -o servomotor -lpigpio -lrt -pthread
```

## Execution

Run the program with elevated privileges:

```bash
sudo ./servomotor
```

To stop the program, press `Ctrl + C`.

## How It Works

1. **Initialization**:
   - Sets up GPIO pins for the button and servomotor.
   - Configures a glitch filter to handle button debounce.

2. **Button Press**:
   - Detects rising edge events (`level == 1`).
   - Applies debounce by ignoring events within 200 ms of the last event.

3. **State Transition**:
   - Uses an FSM to determine the next position (0°, 30°, ..., 180°).
   - Resets to 0° after reaching 180°.

4. **Servomotor Movement**:
   - Converts the position (in degrees) to a PWM signal (in µs).
   - Sends the PWM signal to the servomotor.

5. **Termination**:
   - Stops the PWM signal and releases resources when the program ends.

## Notes

- Ensure the `pigpio` daemon is running before execution:
  ```bash
  sudo pigpiod
  ```
- The program will not function correctly without the required hardware connections.
- Debug messages are printed to the console for each button press and servomotor movement.