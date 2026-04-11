// Import libraries
#include <pigpio.h> // Contains functions to manipulate GPIO and temporizator
#include <stdio.h> // To print LED state in screen
#include <stdlib.h> // To use general functions
#include <stdint.h> // Define data types of fixed size, this allows to work with data sizes predictable (uint8_t && uint32_t)
#include <signal.h> // To manipulate signals (in this program SIGINT [ctrl + C])
#include <inttypes.h> // To print uint32_t type in console
#include <stdbool.h> // To use boolean types
#include <pthread.h> // To manage threads 
#include <time.h> // To use timer_create & itimerspec

#define LED_GPIO 23 // Define number of pin GPIO where's connected LED
#define BUTTON_GPIO 17 // Define number of pin GPIO where's connected button to turn ON or OFF
#define MOTOR_GPIO 18 // Define number of pin GPIO where's connected servomotor to rotate it

/**
* @param sig_atomic_t Special type to work with signal handlers
 * volatile indicates to the compiler that the value can change outside of the main flow of the program
 * This is necessary because the variable is modified inside a function that manipulates signals
 */
volatile sig_atomic_t running = 1;

/**
 * 4. Output Function
 * Calculates microseconds of the actual position in degrees to use it in the function to move the servomotor
 * Applies the movement to the servomotor
 * Prints the actual position of the servomotor after rotated it
 *  */ 
void apply_output(int16_t state);

/**
 * Manipulates signal SIGINT (ctrl + C)
 * 1. Prints in console that the program finished
 * 2. Establish the variable running to 0 to finish while loop
 * @param signal to be manipulated by the O.S.
 *  */ 
void handle_signal(int signal);

/**
 * Reports every second the actual position of the servomotor and the state (secure or alert)
 * Locks mutex when reading position & state to use CRITICAL REGION
 * Finally, unlocks CRITICAL REGION to let other threads use it
 * @param arg argument is a data that can be passed to the callback timer. In this case we don't use it
 *  */ 
void report(union sigval arg);


/**
 * This function is called when pthread_create is called. 
 * Modifies state & emergency_flag depending if button is pressed or not
 * Turns on LED if button was pressed (emergency mode)
 * Prints in console if emergency mode is active
 * Turns off LED if button is not pressed (normal mode)
 * @param arg this parameter is used when you don't want to use global variables. In our case, we ignore it
 */
void *button_monitor(void *arg);

// Mode of the system
typedef enum {
    NORMAL_SYSTEM,
    ALERT_SYSTEM
} system_state_t;

// Direction of the servo
typedef enum {
    GOING_UP,
    GOING_DOWN
} direction_t;

// Servomotor structure
typedef struct {
    int16_t position;
    direction_t direction;
} servo_t;

// Control structure
typedef struct {
    system_state_t state;
    bool emergency_flag;
} control_t;

// System that encompasses both servo & control
typedef struct {
    servo_t servo;
    control_t control;
} system_t;

// Real global variables
system_t sys; // Struct system defined previously

/**
 * Manage mutex in threads
 * Two possibles states:
 * Blocked: possesed by one thread
 * Unblocked: not possesed by any thread
 * Mutex can be different types. In this case we use predeterminated type (normal or faster)
 * 1 thread writes (button)
 * 1 thread writes (servo)
 * 1 context reads (timer)
 *  */
pthread_mutex_t mutex; 

int main(int argc, char const *argv[]) {
    // Initializes all variables previously defined in global scope
    sys.servo.position = 0;
    sys.servo.direction = GOING_UP;
    sys.control.state = NORMAL_SYSTEM;
    sys.control.emergency_flag = false;

    // We use the range given by requirements
    // 500 microseconds = 0 degrees
    // 2500 microseconds = 180 degrees
    // Initial position need's to be 500 microseconds to use it in gpioServe()
    uint32_t initial_position = 500;

    // Initializes button thread
    pthread_t button_thread;

    // Initializes attributes of button thread
    pthread_attr_t attr;

    // Used to store schedule policy. In this case we use Real Time FIFO
    struct sched_param param;

    // Initializes the library of pigpio
    if(gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }

    // Establish in output the PIN 23 of the Rasperry Pi
    gpioSetMode(LED_GPIO, PI_OUTPUT);

    // ------- BUTTON THREAD -------

    /**
     * Initializes mutex to use
     * @param mutex mutex to be initialized
     * @param NULL by defect attributes of mutex initialized
     *  */ 
    pthread_mutex_init(&mutex, NULL);
    
    // Initialize attributes
    pthread_attr_init(&attr);

    // Use explicit configuration. This allows to define a customization of schedule.
    // If this is not present --> You can't define SCHED_FIFO
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    // Planification policy -> Real time FIFO
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // Define priority in maximum to the button thread
    // This allows button thread to be maximum priority & it can interrupt any other threads
    // Button thread activates emergency button
    param.sched_priority = sched_get_priority_max(SCHED_FIFO); 

    // Sets attributes linking with parameter of scheduling defined above
    pthread_attr_setschedparam(&attr, &param);

    /**
     * Creates the button thread, using attributes & function button_monitor() defined in this program
     * @param button_thread to be created
     * @param attr attributes of button thread
     * @param button_monitor customized function defined in this program
     * @param NULL arg of the function. In this case we don't need it
     */
    pthread_create(&button_thread, &attr, button_monitor, NULL);

    // attr is no longer needed --> Release his resources
    pthread_attr_destroy(&attr);

    // Check policy configured above
    int policy;

    pthread_getschedparam(button_thread, &policy, &param);

    printf("Policy: %d | Priority: %d\n", policy, param.sched_priority);

    // ------- -------

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

    // ------- TIMER -------
    // ------- Timer Creation -------

    // Defines what happens when timer expires
    struct sigevent sev;
    
    // When timer expires --> Execute a function in a new thread
    sev.sigev_notify = SIGEV_THREAD;          
    
    // report() is the function to be executed when timer expires
    sev.sigev_notify_function = report;      
    
    // Here you can configure priority & scheduling politic
    sev.sigev_notify_attributes = NULL;
    
    // Id of the timer to be activated
    timer_t timerid;
    
    // Creates a timer in the O.S.
    // CLOCK_REALTIME: time system (real time)
    // &sev: previous configuration setted
    // &timerid: to configure & activate timer
    timer_create(CLOCK_REALTIME, &sev, &timerid);

    // ------- Configure Interval -------
    // Struct that contains time intervals to configure it
    struct itimerspec its;
    // First shot in 1 second
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;

    // Repeates every second
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;

    /**
     * Set interval of timer
     * @param timerid id of the timer to be activated
     * @param flags How to interpretate time (relative or absolute) In this case is 0 (relative). This means that begins
     * in X seconds from now on.
     * @param new_value structure with the times previously defined. 
     * @param old_value saves previous configuration before timer. NULL means that doesn't matter previous value
     *  */ 
    timer_settime(timerid, 0, &its, NULL);

    // Movement task
    while(running) {
        // Lock mutex to read emergency_flag
        pthread_mutex_lock(&mutex);
        
        bool flag = sys.control.emergency_flag;

        // Done reading emergency flag --> Release CRITICAL REGION
        pthread_mutex_unlock(&mutex);

        // This avoids to let running CPU while emergency_flag is true
        if(flag) {
            usleep(50000);  
            continue;
        }

        for( ; sys.servo.position <= 180; ) {
            // Lock mutex to read emergency_flag & increase servo position
            pthread_mutex_lock(&mutex);
            
            // Move only if emergency flag is true
            if(sys.control.emergency_flag) {
                // Unlock mutex to let other processes use thread
                pthread_mutex_unlock(&mutex);    

                // Finish iteration to let other threads do their work
                break;
            }
            // Obtain actual servo position to don't disturb main thread
            int16_t pos = sys.servo.position;

            // Increase shared variable before unlocking CRITICAL REGION
            sys.servo.position++;

            // Unlock mutex to let other processes use thread
            pthread_mutex_unlock(&mutex);

            // Move servomotor and print actual position. This action doesn't require to use mutex
            apply_output(pos);  

            // Do this outside the critical region (mutex woke up here)
            usleep(20000); // To do a soft movement like requirements asked
        }
        for( ; sys.servo.position >= 0; ) {
            // Lock mutex to read emergency_flag & increase servo position
            pthread_mutex_lock(&mutex);
            
            // Move only if emergency flag is true
            if(sys.control.emergency_flag) {
                // Unlock mutex to let other processes use thread
                pthread_mutex_unlock(&mutex);    

                // Finish iteration to let other threads do their work
                break;
            }
            
            // Obtain actual servo position to don't disturb main thread
            int16_t pos = sys.servo.position;

            // Decrease shared variable before unlocking CRITICAL REGION
            sys.servo.position--;

            // Unlock mutex to let other processes use thread
            pthread_mutex_unlock(&mutex);

            // Move servomotor and print actual position. This action doesn't require to use mutex
            apply_output(pos);  

            // Do this outside the critical region (mutex woke up here)
            usleep(20000); // To do a soft movement like requirements asked
        }
    }
    
    // Button thread finishes before cleaning resources
    pthread_join(button_thread, NULL);

    // Release timer created previously
    timer_delete(timerid);         
    
    // Release thread created previously
    pthread_mutex_destroy(&mutex);  
    
    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();

    return 0;
}

void apply_output(int16_t state) {
    // Formula to get microseconds equivalent to degrees moved in servomotor
    // It's calculated using simple rule of three
    uint32_t microseconds = 500 + ((state * 2000) / 180);

    // Applied rotation to servomotor (needs to be passed in microseconds the second parameter)
    gpioServo(MOTOR_GPIO, microseconds);

    // Debug actual position in degrees and microseconds
    printf("Actual position = %d\n", state);
    printf("Actual position = %" PRIu32 "\n", microseconds);
}

void handle_signal(int signal) {
    printf("Program terminated\n");
    running = 0;
}

void report(union sigval arg) {
    (void)arg; // avoid warning by parameter not used

    // Lock mutex to read servo position & system state
    pthread_mutex_lock(&mutex);

    int16_t pos = sys.servo.position;
    system_state_t state = sys.control.state;

    // Done reading global variables --> Release CRITICAL REGION
    pthread_mutex_unlock(&mutex);

    if(state == NORMAL_SYSTEM) {
        printf("[REPORT] Position: %d | State: NORMAL\n", pos);
    } else {
        printf("[REPORT] Position: %d | State: ALERT\n", pos);
    }
}

void *button_monitor(void *arg) {
    while(running) {
        // Reads button state
        int state = gpioRead(BUTTON_GPIO);
        
        // Lock mutex to read emergency_flag
        pthread_mutex_lock(&mutex);
        
        bool flag = sys.control.emergency_flag;

        // Done reading emergency flag --> Release CRITICAL REGION
        pthread_mutex_unlock(&mutex);

        // This avoids to let running CPU while emergency_flag is true
        if(flag) {
            usleep(50000);  
            continue;
        }
        
        // Lock mutex to modify emergency_flag & state's system
        pthread_mutex_lock(&mutex);
        if(state) {
            if(!sys.control.emergency_flag) {
                // Change state of the system to alert or emergency
                sys.control.state = ALERT_SYSTEM;
                
                // Emergency flag turned on
                sys.control.emergency_flag = true;
    
                // Done modifing global variables --> Release CRITICAL REGION
                pthread_mutex_unlock(&mutex);
    
                // Turn on warning LED
                gpioWrite(LED_GPIO, state);
        
                // Print in console because requeriments say it
                printf("[ALERT] Emergency stop activated - Latency detected\n");
            } else {
                // Release CRITICAL REGION to avoid dead lock
                pthread_mutex_unlock(&mutex);
            }
        } else {
            // Change state of the system to normal 
            sys.control.state = NORMAL_SYSTEM;
            
            // Emergency flag turned off
            sys.control.emergency_flag = false;

            // Done modifing global variables --> Release CRITICAL REGION
            pthread_mutex_unlock(&mutex);

            // Turn on warning LED
            gpioWrite(LED_GPIO, state);
        }

        // Do this outside the critical region (mutex woke up here)
        usleep(20000); // To do a soft movement like requirements asked
    }

    // Pthreads always returns void*
    return NULL;
}