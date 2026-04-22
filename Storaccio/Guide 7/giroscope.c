#include <stdio.h>   // Use printf, perror ...
#include <stdlib.h>  // Use exit, malloc ...
#include <stdint.h>  // Manage uint8_t
#include <pigpio.h>
#include <unistd.h> // To use usleep()
#include <pthread.h> // To create new threads
#include <sched.h> // To use schedulers politics
#include <signal.h> // To manage signal SIGINT
#include <mqueue.h> // To manage queue messages
#include <errno.h>

// Datasheet of MPU6050 sensor is defined with values that are written in this program
#define I2C_BUS 1
#define MPU6050_ADDR 0x68
#define PWR_MGMT_1 0x6B

// Define registers in bytes to read data from sensor MPU6050
#define ACCEL_XOUT_H 0x3B

// To manage sensibility of sensor
#define ACCEL_CONFIG 0x1C

// Define number of pin GPIO where's connected servomotor to rotate it
#define MOTOR_GPIO 18 

// Sample quantity of filter
#define N 10

// Global angle for servomotor
float global_angle_x = 0.0;

// To let program finish when CTRL + C touched
volatile sig_atomic_t running = 1;

// Mutex to use it in threads
pthread_mutex_t mutex;

// Descriptor to let taskA & taskB to let share between them
mqd_t queue;

/**
 * Define global shared variables of three axis to compute & measure with the sensor & the UI
 * Used for the queue message & protected variables for mutex
 */
typedef struct {
    int16_t x, y, z;
} ImuData;

/**
 * Productor (Adquisition)
 * 1. Reads raw data X, Y, Z of MPU6050 through I2C (This lecture is done with 100 Hz (every 10 ms))
 * 2. Sends this read data to the message queue with mq_send()
 */
void* taskA(void* arg);

/** 
 * Consumer (Prosecution)
 * 1. Receives queue data with mq_receive()
 * 2. Applies moving average filter
 * 3. Prints to stdout in CSV format for the pipe with Python
 * 
*/
void* taskB(void* arg);

/**
 * Actuator (Optional)
 * 1. Reads filtered angle of X axis
 * 2. Commands to the slave SG90 with gpioServo()
 * 3. Mutex needed to access shared variable with thread B
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

    // Defines struct & attributes of queue message
    struct mq_attr attr;
    attr.mq_flags   = 0; // Flags by defect in 0
    attr.mq_maxmsg  = 10; // Maximum 10 messages waiting
    attr.mq_msgsize = sizeof(ImuData); // Size of every message
    attr.mq_curmsgs = 0; // Actual number of messages in the queue

    
    /**
     * Open queue after configuring attributes of it
     * @param name pointer to a string that specifies name of message queue
     * @param oflag flags that determine access mode and creation mode
     * @param mode file permission for the new queue
     * @param attr passes the memory address of attributes previously defined
     */
    queue = mq_open("/imu_queue", O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);

    // Handle error of opened queue
    if(queue == (mqd_t)-1) {
        perror("Error opening queue");
        return EXIT_FAILURE;
    }

    // Initializes the library of pigpio
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }

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
    if(pthread_attr_setschedpolicy(&attrA, SCHED_FIFO) != 0) {
        perror("schedpolicy A error");
    }

    if(pthread_attr_setschedpolicy(&attrB, SCHED_FIFO) != 0) {
        perror("schedpolicy B error");
    }

    if(pthread_attr_setschedpolicy(&attrC, SCHED_OTHER) != 0) {
        perror("schedpolicy C error");
    }

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

    // Clean queue created in this program
    mq_close(queue);
    mq_unlink("/imu_queue");
    
    return 0;
}

void* taskA(void* arg) {
    // Define instance of three axis (x, y, z)
    ImuData rawData;

    // Datasheet of MPU6050 sensor is defined with values that are written in this program
    /**
     * Opens sensor to allow writing and reading it.
     * Opens connection with a I2C device
     * @param bus
     * @param address slave address
     * @param flags generally 0 (it's used for special configurations)
     */
    int handle = i2cOpen(I2C_BUS, MPU6050_ADDR, 0);
    
    // Handle error of opening MPU6050 sensor
    if(handle < 0) {
        fprintf(stderr, "Error opening I2C on MPU6050 sensor\n");
        return NULL;
    }

    // Initially the dispositive is slept
    // Before reading some data, it's necessary to write in the register PWR_MGMT_1 to wake it up
    // 0x00 wakes it up
    // 1. Initialize MPU6050 sensor
    if(i2cWriteByteData(handle, PWR_MGMT_1, 0x00) != 0) {
        fprintf(stderr, "Error initializing MPU6050\n");
        i2cClose(handle);
        return NULL;
    }

    // Configure accelerometer in ±2g (AFS_SEL = 0)
    if(i2cWriteByteData(handle, ACCEL_CONFIG, 0x00) != 0) {
        fprintf(stderr, "Error configuring ACCEL_CONFIG\n");
        i2cClose(handle);
        return NULL;
    }

    while(running) {
        // 2. Read 6 bytes
        uint8_t data[6];

        /**
         * Reads data from a I2C device starting from ACCEL_XOUT_H (0x3B)
         * Reads the following data: {0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40}
         * This means, the following 6 bytes
         * Called in another way: {ACEL_XOUT_H, ACCEL_XOUT_L, ACEL_YOUT_H, ACEL_YOUT_L, ACEL_ZOUT_H, ACEL_ZOUT_L}
         * @param handle id obtained of device opened previously
         * @param buffer buffer when data read will be saved
         * @param count bytes quantity to read
         */
        int bytes = i2cReadI2CBlockData(handle, ACCEL_XOUT_H, data, 6);

        // Handle reading error of I2C
        if(bytes < 0) {
            perror("I2C read error");
            continue;
        }
        
        // Handle incomplete error
        if(bytes != 6) {
            // This avoids breaking pipe with Python
            fprintf(stderr, "Incomplete read\n");
            continue;
        }

        // Every axis occupies 2 consecutive bytes
        // This means: value = (high_byte << 8) | low_byte
        rawData.x = (data[0] << 8) | data[1];
        rawData.y = (data[2] << 8) | data[3];
        rawData.z = (data[4] << 8) | data[5];

        /**
         * Send read data through sensor.
         * @param mqdes queue's descriptor
         * @param msg_ptr points to the data read
         * @param msg_len byte's size of data to read
         * @param msg_prio message's priority
         */
        if(mq_send(queue, (char*)&rawData, sizeof(ImuData), 0) < 0) {
            // Full queue ==> discard actual sample
            if(errno == EAGAIN) {
                continue;
            }

            // Catches real error if queue is not full
            perror("Error sending data to queue");
            continue;
        }

        // Requirements asks for 100 Hz (every 10 ms)
        usleep(10000);
    }

    // Close I2C protocol
    i2cClose(handle);

    return NULL;
}

void* taskB(void* arg) {
    // Circular buffers for every axis
    float buf_x[N] = {0};
    float buf_y[N] = {0};
    float buf_z[N] = {0};

    int count = 0;

    // Circular index
    int idx = 0;

    // Variable when it saves data received from queue
    ImuData raw;

    while(running) {
        // Receive queue's data (blocks it until there's a message)
        // Same parameters that mq_send
        if(mq_receive(queue, (char*)&raw, sizeof(ImuData), NULL) < 0) {
            // This means error of empty queue
            if(errno == EAGAIN) {
                usleep(1000); // Little wait to keep going after it
                continue;
            }
            
            // If errno != EAGAIN catches real error
            perror("Error receiving from queue");
            continue;
        }

        // Converts to float (sensor returns accounts, not degrees
        // With AFS_SEL=0 (±2g), sensibility is 16384 LSB/g
        float x = raw.x / 16384.0;
        float y = raw.y / 16384.0;
        float z = raw.z / 16384.0;

        // Save it in circular buffer
        buf_x[idx] = x;
        buf_y[idx] = y;
        buf_z[idx] = z;

        // Advance circular index
        idx = (idx + 1) % N;

        // Begin calculus of averages
        float sum_x = 0, sum_y = 0, sum_z = 0;

        for(int i = 0; i < N; i++) {
            sum_x += buf_x[i];
            sum_y += buf_y[i];
            sum_z += buf_z[i];
        }

        if(count < N) count++;

        // Calculate averages
        float fx = sum_x / count;
        float fy = sum_y / count;
        float fz = sum_z / count;

        // (Optional) Save filtered angle X for the servo
        pthread_mutex_lock(&mutex);
        global_angle_x = fx * 90.0;  // Mapping to approximated degrees
        pthread_mutex_unlock(&mutex);

        // Print CSV to stdout for the pipe with Python
        printf("%.4f,%.4f,%.4f\n", fx, fy, fz);

        // Very important for the pipe to flow
        fflush(stdout);  
    }

    return NULL;
}

void* taskC(void* arg) {
     while(running) {
        // Initialize angle of actual iteration
        float angle;

        // Set local variable angle with shared variable angle of x axis
        pthread_mutex_lock(&mutex);
        angle = global_angle_x;
        pthread_mutex_unlock(&mutex);

        // If angle > 90° || angle < -90°
        // This prevents a damage in servo or jitter
        if(angle > 90) angle = 90;
        if(angle < -90) angle = -90;

        // Mapping it to servo (500–2500 µs)
        int pulse = 1500 + (angle * 1000.0 / 90.0);

        // Move servomotor to actual pulse (position)
        gpioServo(MOTOR_GPIO, pulse);

        // Tipical 50 Hz of servomotor
        usleep(20000);
    }

    // Moves servomotor to initial position after cleaning program
    gpioServo(MOTOR_GPIO, 0); 

    return NULL;
}

void handle_sigint(int sig) {
    running = 0;
}