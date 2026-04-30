#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
// #include <pigpio.h>
#include <signal.h>
#include <errno.h>

// File where happens the communication with client & server with sockets
#define SOCKET_PATH "/tmp/control_led.sock"
#define BUFFER_SIZE 256

#define LED_PIN 17

// Shared global variable to manage led state
int led_state = 0;

// To let program finish when CTRL + C touched
volatile sig_atomic_t running = 1;

// Mutex to use it in threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Manages SIGINT signal.
 * CTRL + C combination needed to use this signal
 * Sets variable running to 0
 */
void handle_sigint(int sig);

/**
 * Function executed by every thread - Attends one client
 */
void *worker(void *arg);

int main() {
    // File descriptors to realize the communication
    int server_fd, client_fd;

    // To realize bind()
    struct sockaddr_un addr;

    // Initialize thread
    pthread_t thread;

    // Initializes the library of pigpio
    /* if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    } */

    // Establish in output the PIN 17 of the Rasperry Pi to handle ventilation fan
    // gpioSetMode(LED_PIN, PI_OUTPUT);

    // Handles signal SIGINT (CTRL + C)
    signal(SIGINT, handle_sigint);

    /**
     * 1. Create socket
     * @param AF_UNIX direction's family used for communication between processes executed in the same local machine
     * @param SOCK_STREAM socket's type oriented to connection. Operates in the SO kernel
     * @param 0 automatic protocol
     */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // Handle's error
    if (server_fd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    printf("Socket created OK\n");

    // 2. Delete file if it's still in the previous execution
    unlink(SOCKET_PATH);

    // 3. Configure address
    // Sets sizeof(addr) bytes of &addr to 0 ==> Initialize 0 bytes
    memset(&addr, 0, sizeof(addr));

    // Sets AF_UNIX direction's family to addr
    addr.sun_family = AF_UNIX;

    /**
     * Sets actual address SOCKET_PATH to addr 
     * This ensures copying all SOCKET_PATH correctly except char '\0'
     * @param addrsun_path destiny
     * @param SOCKET_PATH source
     * @param sizeof(addr.sun_path) - 1 characters
     */
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    
    /**
     * 4. Bind: associate socket to the file
     * Gives to the socket address with size to the file descriptor
     * @param server_fd file descriptor' server
     * @param addr socket address
     * @param sizeof(addr) size of socket addr
     */
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error en bind");
        close(server_fd);
        exit(1);
    }
    printf("Bind OK — socket in %s\n", SOCKET_PATH);

    /**
     * 5. Listen: listening server (maximum 5 connections in queue)
     * @param server_fd file descriptor's server
     * @param 5 maximum connections in queue before furter requests are refused
     */
    if (listen(server_fd, 5) < 0) {
        perror("Error in listen");
        close(server_fd);
        exit(1);
    }
    printf("Listening connections...\n");

    // 6. Accepts clients & throws threads
    while (running) {
        /**
         * 6. Accept: wait & accept one client (for now)
         * Await a connection on socket FD
         * When a connection arrives, open a new socket to communicate with it
         * @param server_fd server's file descriptor
         * @param NULL address client
         * @param NULL address client size
         * Both parameters in NULL means that's it's useless saving this information because we already know the address because it's local
         * This information is useful when it's happens another type of communication that is not SOCK_STREAM
         */
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            // Handle CTRL + C signal
            if (errno == EINTR) break;
            perror("Error in accept");
            break;
        }
        printf("[Main] Client connected! Beggining thread...\n");

        // Important: we use malloc for the file descriptor
        // If we pass &client_fd directly, the loop could step on it before that thread reads it
        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;

        // Create thread passing previous file descriptor & using previous worker function
        if (pthread_create(&thread, NULL, worker, fd_ptr) != 0) {
            perror("Error creating thread");
            free(fd_ptr); // Free's up resources of fd_ptr
            close(client_fd);
            continue;
        }

        // Detach: thread cleans automatically alone when finishing
        pthread_detach(thread);
    }
    
    // 8. Close connection
    // Close file descriptor server
    close(server_fd);

    // Remove the link name SOCKET_PATH
    unlink(SOCKET_PATH);

    // gpioTerminate();

    return 0;
}

void *worker(void *arg) {
    // Recovers file descriptor client that has been passed
    int client_fd = *((int *)arg);

    // Free's up memory that we asked in main
    free(arg); 

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    // To save data received from server
    int read_bytes;

    // 7. Read message's client
    // Read command client
    // Initialize buffer of size BUFFER_SIZE to 0
    memset(buffer, 0, BUFFER_SIZE);

    /**
     * Read BUFFER_SIZE - 1 into buffer from client_fd. Return the
     * number read, -1 for errors or 0 for EOF.
     * @param client_fd file descriptor' client
     * @param buffer filled with bytes read from file descriptor's client
     * @param BUFFER_SIZE - 1 size of bytes read
     */
    read_bytes = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (read_bytes > 0) {
        printf("[Hilo] Received command: '%s'\n", buffer);

        // All modifications/lectures of led_state needs to be inside critical region
        // Nobody can interrupt in the middle of the process
        pthread_mutex_lock(&mutex);

        // For now we respond depending on command, without pigpio neither mutex for now
        // If buffer has content "ON"
        if (strcmp(buffer, "ON") == 0) {
            led_state = 1;

            // Writes in response the content "LED_OK: ON of sizeof(response)"
            snprintf(response, sizeof(response), "LED_OK: ON");

            // Turns ON LED
            // gpioWrite(LED_PIN, PI_HIGH);

            // If buffer has content "OFF"
        } else if (strcmp(buffer, "OFF") == 0) {
            led_state = 0;            

            // Writes in response the content "LED_OK: OFF of sizeof(response)"
            snprintf(response, sizeof(response), "LED_OK: OFF");

            // Turns OFF LED
            // gpioWrite(LED_PIN, PI_LOW);

            // If buffer has content "STATUS"
        } else if (strcmp(buffer, "STATUS") == 0) {
            // Just reading, not writing like both previous ways
            snprintf(response, sizeof(response), "LED_STATUS: %s", led_state ? "ON" : "OFF");
        } else {
            // Handle no command written
            snprintf(response, sizeof(response), "ERROR: INVALID_COMMAND");
        }
        pthread_mutex_unlock(&mutex);

        // Debug actual situation
        printf("[Hilo] State: %d — Response: '%s'\n", led_state, response);
    
        // Write in client the response content of response size
        write(client_fd, response, strlen(response));
    
        printf("[Hilo] Attended client, thread terminated\n");
    } else {
        printf("[Hilo] Client disconnected without sending data\n");
    }
    close(client_fd);
    
    return NULL;
}

void handle_sigint(int sig) {
    running = 0;
}