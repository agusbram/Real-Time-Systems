#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/control_led.sock"
#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    // File descriptors to realize the communication
    int client_fd;

    // To realize bind()
    struct sockaddr_un addr;

    // To save content read & send it
    char buffer[BUFFER_SIZE];

    // To save data received from client
    int read_bytes;

    // 1. Verify if an argument has been passed (ON, OFF or STATUS)
    if (argc < 2) {
        printf("Use: ./client <ON|OFF|STATUS>\n");
        exit(1);
    }

    /**
     * 2. Create socket
     * @param AF_UNIX direction's family used for communication between processes executed in the same local machine
     * @param SOCK_STREAM socket's type oriented to connection. Operates in the SO kernel
     * @param 0 automatic protocol
     */
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    printf("Socket created OK\n");

    // 3. Configure address server
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
     * 4. Connect: conectarse al servidor
     * Open a connection on socket client_fd to peer at &addr (which sizeof(addr) bytes long).
     * @param client_fd file descriptor's client
     * @param addr socket address
     * @param sizeof(addr) size of socket addr
     */
    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error connecting to the server");
        close(client_fd);
        exit(1);
    }
    printf("Connected to the server!\n");

    /**
     * 5. Send command (argv[1] is "ON", "OFF" or "STATUS")
     * @param client_fd file descriptor's client
     * @param argv[1] command written in console (ON, OFF, STATUS)
     * @param strlen(argv[1]) command written size
     */
    if (write(client_fd, argv[1], strlen(argv[1])) < 0) {
        perror("Error sending command");
        close(client_fd);
        exit(1);
    }
    printf("Comando send: '%s'\n", argv[1]);

    // 6. Wait & read server response
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
        printf("Server response: '%s'\n", buffer);
    } else {
        printf("Response was not received\n");
    }

    // 7. Close connection
    close(client_fd);

    return 0;
}