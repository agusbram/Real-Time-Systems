#include <stdio.h>   // Use printf, perror ...
#include <stdlib.h>  // Use exit, malloc ...
#include <stdint.h>  // Manage uint8_t
#include <pigpio.h>
#include <unistd.h> // To use usleep()
#include <pthread.h> // To create new threads
#include <sched.h> // To use schedulers politics
#include <signal.h> // To manage signal SIGINT

// Datasheet of AHT10 sensor is defined with values that are written in this program
#define I2C_BUS 1
#define AHT10_ADDR 0x38

// Define PIN of ventilation to turn it ON or OFF
#define FAN_PIN 17

// Define states of machine's state
typedef enum {
    REPOSE,
    ALERT,
    VENTILATION
} state_t;

// To let program finish when CTRL + C touched
volatile sig_atomic_t running = 1;

// Shared variable of temperature
float global_temp = 0;

// Shared variable of state
state_t global_state = REPOSE;

// Mutex to use it in threads
pthread_mutex_t mutex;

/**
 * 1. Initializes protocol I2C communication
 * 2. Measures ambience's temperature
 * 3. Translates temperature measured to temperature relevant of sensor in his range
 */
void* taskA(void* arg);

/**
 * Finite State Machine that is responsible of manage states of air conditioning 
 * Also, turns ON or OFF ventilation, alert & repose mode
 */
void* taskB(void* arg);

/**
 * Responsible of diagnosis of air conditioning
 * Print's every situation that passes in the air conditioning
 */
void* taskC(void* arg);

/**
 * Manages SIGINT signal.
 * CTRL + C combination needed to use this signal
 * Sets variable running to 0
 */
void handle_sigint(int sig);

int main(int argc, char const *argv[]) {
    // Defines three threads
    pthread_t thA, thB, thC;

    // Defines attributes of three threads
    pthread_attr_t attrA, attrB, attrC;

    // Defines schedulers parameters of three threads
    struct sched_param paramA, paramB, paramC;

    // Initializes the library of pigpio
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }

    // Establish in output the PIN 17 of the Rasperry Pi to handle ventilation fan
    gpioSetMode(FAN_PIN, PI_OUTPUT);

    // Turn OFF starting ventilation mode 
    gpioWrite(FAN_PIN, 0);

    // Handles signal SIGINT (CTRL + C)
    signal(SIGINT, handle_sigint);

    // Initialize mutex
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Error initializing mutex");
        return EXIT_FAILURE;
    }

    
    // Initializes three attributes of three threads
    // Also, handles error initializing them
    if(pthread_attr_init(&attrA) != 0) {
        perror("Error initializing thread A");
        return EXIT_FAILURE;
    }

    if(pthread_attr_init(&attrB) != 0) {
        perror("Error initializing thread B");
        return EXIT_FAILURE;
    }

    if(pthread_attr_init(&attrC) != 0) {
        perror("Error initializing thread C");
        return EXIT_FAILURE;
    }

    // Defines SCHED_FIFO schedule policy of three threads
    pthread_attr_setschedpolicy(&attrA, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attrB, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attrC, SCHED_OTHER);

    // Necessary to make personalized threads
    pthread_attr_setinheritsched(&attrA, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attrB, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attrC, PTHREAD_EXPLICIT_SCHED);

    // Defines policies of three threads
    paramA.sched_priority = 80; // High
    paramB.sched_priority = 60; // Medium
    paramC.sched_priority = 0; // Low
    
    // Sets attributes & parameters of three threads
    pthread_attr_setschedparam(&attrA, &paramA);
    pthread_attr_setschedparam(&attrB, &paramB);
    pthread_attr_setschedparam(&attrC, &paramC);

    // Creates three threads with previous attributes & parameters
    // Also, handles error creating them
    if(pthread_create(&thA, &attrA, taskA, NULL) != 0) {
        perror("Error creating thread A");
        return EXIT_FAILURE;
    } 
    if(pthread_create(&thB, &attrB, taskB, NULL) != 0) {
        perror("Error creating thread B");
        return EXIT_FAILURE;
    }

    if(pthread_create(&thC, &attrC, taskC, NULL) != 0) {
        perror("Error creating thread C");
        return EXIT_FAILURE;
    }

    // Wait to finish three threads
    pthread_join(thA, NULL);
    pthread_join(thB, NULL);
    pthread_join(thC, NULL);

    // Free's up resources of used mutex
    pthread_mutex_destroy(&mutex);

    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();
    
    return 0;
}

void* taskA(void* arg) {
    // Datasheet of AHT10 sensor is defined with values that are written in this program
    /**
     * Opens sensor to allow writing and reading it.
     * Opens connection with a I2C device
     * @param bus
     * @param address slave address
     * @param flags generally 0 (it's used for special configurations)
     */
    int handle = i2cOpen(I2C_BUS, AHT10_ADDR, 0);
    
    // Handle error of opening AHT10 sensor
    if(handle < 0) {
        fprintf(stderr, "Error opening I2C on AHT10 sensor\n");
        return NULL;
    }

    // Datasheet defines this 6 steps written below
    // 1. Wait 20 ms after turning on
    usleep(20000);

    // 2. Initialize AHT10 sensor
    // Initialization command
    uint8_t init_cmd[3] = {0xE1, 0x08, 0x00};

    /**
     * Sends data to the I2C device
     * @param handle id obtained of device opened previously
     * @param buffer bytes array with data to send
     * @param count bytes quantity to send
     */
    if(i2cWriteDevice(handle, init_cmd, 3) != 0) {
        fprintf(stderr, "Error sending data of AHT10\n");
        i2cClose(handle);
        return NULL;
    } 

    // Little delay
    usleep(10000);

    while(running) {
        // 3. Measure command
        uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};

        if(i2cWriteDevice(handle, measure_cmd, 3) != 0) {
            fprintf(stderr, "Error sending data of AHT10\n");
            continue;
        }

        // 4. Wait minimum 75 ms
        usleep(80000);

        // 5. Read 6 bytes
        uint8_t data[6];

        /**
         * Reads data from a I2C device
         * @param handle id obtained of device opened previously
         * @param buffer buffer when data read will be saved
         * @param count bytes quantity to read
         */
        int bytes = i2cReadDevice(handle, data, 6);

        // Handle error that happens when reading sensor
        if(bytes != 6) {
            printf("Error reading sensor\n");

            continue;
        }

        // 6. Verify bit 7 (busy flag)
        if (data[0] & 0x80) {
            // 1: Busy sensor
            // 0: Valid data
            printf("Sensor busy\n");

            continue;
        }

        // 7. Process temperature
        uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];

        // Sensor uses this range of temperature: [-50 °C, 150 °C]
        // Sensor returns a binary number of 20 bits
        // That's why you need to convert it into a relevant value with this formula given in the datasheet
        float temperature = ((raw_temp * 200.0) / 1048576.0) - 50;

        // Lock critical region to save global temperature
        pthread_mutex_lock(&mutex);

        // Save actual temperatura to global temperature
        global_temp = temperature;

        // Unlock critical region to let other threads use this shared variable
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }

    // Close I2C protocol
    i2cClose(handle);

    return NULL;
}

void* taskB(void* arg) {
    // Starts mode to REPOSE
    state_t state = REPOSE;

    // Defines start time at 0 of alert & ventilation
    uint32_t t_alert_start = 0;
    uint32_t t_ventilation_start = 0;

    while(running) {
        // Initialize local variable temperature
        float temp;

        // Lock critical region to read global temperature
        pthread_mutex_lock(&mutex);

        // Save global temperature to local variable
        temp = global_temp;

        // Let other threads use critical region
        pthread_mutex_unlock(&mutex);

        // Actual time
        uint32_t now = gpioTick();

        // Initiate FSM (finite state machine) to manage ventilation
        switch(state) {
            // Ventilation is OFF
            case REPOSE:
                // Temperature high
                if(temp > 30.0) {
                    // Turn ON alert
                    state = ALERT;

                    // Set start time of alert ON in the actual time
                    t_alert_start = now;
                }
                break;
            
            // Ventilation is in alert
            case ALERT:
                // Transition: ALERT -> REPOSE
                if(temp <= 30.0) {
                    state = REPOSE;
                } else {
                    // If alert is turned ON more than 60 seconds
                    if((now - t_alert_start) >= 60000000) {
                        // Turn ON ventilation
                        gpioWrite(FAN_PIN, 1);

                        state = VENTILATION;

                        // Set start time of ventilation mode ON in the actual time
                        t_ventilation_start = now;
                    }
                }
                break;

            // Ventilation is ON
            case VENTILATION:
                // Exit ventilation condition
                if(temp < 25.0) {
                    // Transition: VENTILATION -> REPOSE
                    gpioWrite(FAN_PIN, 0);

                    state = REPOSE;
                } else if((now - t_ventilation_start) >= 120000000) {
                    // If ventilation is turned ON for more than 120 seconds
                    gpioWrite(FAN_PIN, 0);
                    
                    state = REPOSE;
                }
                break;
        }

        // Lock critical region to set global state
        pthread_mutex_lock(&mutex);

        // Set actual state to make diagnosis en thread C (taskC)
        global_state = state;

        // Unlock critical region to allow other threads use global state
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }
    return NULL;
}

void* taskC(void* arg) {
    while(running) {
        // Initialize temperature & state
        float temp;
        state_t state;

        // Lock critical region to read global temperature & state
        pthread_mutex_lock(&mutex);
        temp = global_temp;
        state = global_state;

        // Unlock critical region
        pthread_mutex_unlock(&mutex);

        // Debug actual temperature measured by sensor
        printf("=== SYSTEM STATUS ===\n");
        printf("Temp: %.2f °C\n", temp);

        // Debug actual state
        switch(state) {
            case REPOSE: printf("State: REPOSE\n"); break;
            case ALERT: printf("State: ALERT\n"); break;
            case VENTILATION: printf("State: VENTILATION\n"); break;
        }

        // Separator
        printf("=====================\n\n");

        sleep(5);
    }
    
    return NULL;
}

void handle_sigint(int sig) {
    running = 0;
}