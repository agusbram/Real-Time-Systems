// Import libraries
#include <pigpio.h> // Contains functions to manipulate GPIO and temporizator
#include <stdio.h> // To print LED state in screen
#include <stdlib.h> // To use general functions
#include <stdint.h> // Define data types of fixed size, this allows to work with data sizes predictable (uint8_t && uint32_t)
#include <signal.h> // To manipulate signals (in this program SIGINT [ctrl + C])
#include <inttypes.h> // To print uint32_t type in console

#define BUTTON_GPIO 17 // Define number of pin GPIO where's connected button to turn ON or OFF
#define MOTOR_GPIO 18 // Define number of pin GPIO where's connected servomotor to rotate it

/**
* @param sig_atomic_t Special type to work with signal handlers
 * volatile indicates to the compiler that the value can change outside of the main flow of the program
 * This is necessary because the variable is modified inside a function that manipulates signals
 */
volatile sig_atomic_t running = 1;

/**
 * Manipulates signal SIGINT (ctrl + C)
 * 1. Prints in console that the program finished
 * 2. Establish the variable running to 0 to finish while loop
 * @param signal to be manipulated by the O.S.
 *  */ 
void handle_signal(int signal);

/**
 * 2. Event
 * Handles all of the events that occurs when logic level changes in GPIO. This is handled in an asynchronous way.
 * @param gpio number of physical pin of GPI that generated the event handled
 * @param level indicates actual logic level of GPIO in the moment that happens the event handled
 * @param tick time in microseconds that indicates when happened the change of the state of GPIO
 *  */ 
void button_callback(int gpio, int level, uint32_t tick);

/** 
 * 3. Transition Function
 * This is the movement of the servomotor requested in requirements to rotate it correcly
 * @param current_state actual state of the servomotor (in degrees)
 * */ 
uint8_t next_state(uint8_t current_state);

/**
 * 4. Output Function
 * Calculates microseconds of the actual position in degrees to use it in the function to move the servomotor
 * Applies the movement to the servomotor
 * Prints the actual position of the servomotor after rotated it
 *  */ 
void apply_output(uint8_t state);

// Needed to do the conversion to microseconds and rotate servomotor
// State of the system (servomotor position)
// It's expressed in degrees
// Finite set: {0,30,60,...,180}
// 1. State
uint8_t state = 0;

// Previous event produced to use it in the debounce requested in requirements to let servomotor work correctly
uint32_t previous_time = 0;

int main(int argc, char const *argv[]) {
    // We use the range given by requirements
    // 500 microseconds = 0 degrees
    // 2500 microseconds = 180 degrees
    // Initial position need's to be 500 microseconds to use it in gpioServe()
    uint32_t initial_position = 500;

    // Initializes the library of pigpio
    if(gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }

    // Establish in input the PIN 17 of the Rasperry Pi to handle button
    gpioSetMode(BUTTON_GPIO, PI_INPUT);

    // Establish in output the PIN 18 of the Rasperry Pi to handle servomotor
    gpioSetMode(MOTOR_GPIO, PI_OUTPUT);

    // PIN receives electric sound of the ambience
    // It's necessary to reduce that sound to keep away from float state, this means that GPIO can read 0 1 0 1 1 0 0 1 even if you haven't pressed the button
    // @param PI_PUD_DOWN (Pull-up) connects PIN to VCC (3.3V) through a resistance.
    // Pull down means:
    //  When button is pressed ==> pin mantains in 1 logic
    //  When button is not pressed ==> pin is connected to GROUND and passes to 0 logic.
    gpioSetPullUpDown(BUTTON_GPIO, PI_PUD_DOWN);

    // This function allows to pigpio to wait until the change state is stable:
    // time(ms)    GPIO
    // 0             1
    // 1             0
    // 2             1
    // 3             0
    // 4             1
    // 5             0
    // 15            0
    // 10000 microseconds = 10 miliseconds
    // Normal bouncing typical mechanics of buttons is between 1 - 5 miliseconds. In the worst case is aproximately 10 miliseconds.
    // This filter eliminates sound (like interferences in a microphone)
    // Is in level of hardware/driver, cleans signal and avoids electric sound
    gpioGlitchFilter(BUTTON_GPIO, 10000);

    // Second parameter need's to be in microseconds
    // This function initializes the state of the servomotor. It's like a gpioWrite()
    gpioServo(MOTOR_GPIO, initial_position);


    // Handles ctrl + C interruption
    signal(SIGINT, handle_signal);

    /**
     * It allows you to register a callback function that executes automatically when a logic level change is detected on a GPIO pin.
     * This mechanism avoids the use of continuous polling and allows for reactive handling of hardware events.
     *  */ 
    gpioSetAlertFunc(BUTTON_GPIO, button_callback);

    while(running) {
        // Program sleeps 100 miliseconds and then iterates again
        // This avoids busy waiting and mantains the program alive while pigpio handles callbacks
        // Now the program doesn't consumes CPU unnecessarily
        // 100.000 microsecond = 100 miliseconds
        // usleep = sleep but usleep is for microseconds and sleep is for seconds
        usleep(100000);
    }

    // Second parameter need's to be in microseconds
    // Servomotor is stopped. Signal PMW is stopped
    gpioServo(MOTOR_GPIO, 0);

    // This finishes reading interruptions/events of button
    gpioSetAlertFunc(BUTTON_GPIO, NULL);

    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();

    return 0;
}


void button_callback(int gpio, int level, uint32_t tick) {
    // This is not necessary because only exists one button in this program. 
    // This allows the function to be reusable
    if(gpio != BUTTON_GPIO) return;

    // Rising flank requested in requirements
    if(level != 1) return;

    // Debounce time to reduce false shots because of the sound of the mechanic button
    // tick returns exact instant of interrupt event produced in this callback or interruption
    if(tick - previous_time < 200000) return;

    // Save the time of the previous event produced to use it in the debounce
    previous_time = tick;

    // Debug to show the event interruption
    printf("Button pressed (event)\n");

    // 5. FSM Motor (Finite State Machine)
    // Applies next position to the servomotor
    state = next_state(state);

    // Debug the actual situation of the servomotor event
    if(state == 0) {
        printf("Limit exceeded, resetting to 0°\n");
    } else {
        printf("Degrees increased by 30\n");
    }

    // 5. FSM Motor (Finite State Machine)
    // Applies rotation to the servomotor with the actual position
    apply_output(state);
}

void handle_signal(int signal) {
    printf("Program terminated\n");
    running = 0;
}

uint8_t next_state(uint8_t current_state) {
    if(current_state < 180) {
        return current_state + 30;
    } else {
        return 0;
    }
}

void apply_output(uint8_t state) {
    // Formula to get microseconds equivalent to degrees moved in servomotor
    // It's calculated using simple rule of three
    uint32_t microseconds = 500 + ((state * 2000) / 180);

    // Applied rotation to servomotor (needs to be passed in microseconds the second parameter)
    gpioServo(MOTOR_GPIO, microseconds);

    // Debug actual position in degrees and microseconds
    printf("Actual position = %d\n", state);
    printf("Actual position = " PRIu32 "\n", microseconds);
}