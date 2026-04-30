# LED Control Socket Communication System

## Overview

This project implements a **client-server architecture** using Unix domain sockets for inter-process communication (IPC) on a local machine. The server manages a shared LED state and handles concurrent client requests using multi-threading. Clients can send commands to control or query the LED state.

## Features

- **Unix Domain Sockets**: Efficient local IPC mechanism for client-server communication
- **Multi-threaded Server**: Each client connection is handled by a dedicated thread
- **Thread-Safe Operations**: Uses mutex locks to ensure thread-safe access to shared resources
- **LED State Management**: Supports ON/OFF control and STATUS queries
- **Signal Handling**: Graceful shutdown using CTRL+C (SIGINT)
- **Buffer-Based Communication**: 256-byte buffers for message exchange

## Project Structure

```
Guide 8/
├── acting_server.c      # Server application
├── acting_client.c      # Client application
└── README.md           # This file
```

## Compilation

### Prerequisites
- GCC compiler
- POSIX-compliant system (Linux, macOS, etc.)
- Build tools: `gcc` or `gcc-12`

### Build Instructions

#### Option 1: Using GCC
```bash
# Compile the server
gcc -o acting_server acting_server.c -lpthread

# Compile the client
gcc -o acting_client acting_client.c
```

#### Option 2: Using GCC-12 (Recommended)
```bash
# Compile the server
gcc-12 -o acting_server acting_server.c -lpthread

# Compile the client
gcc-12 -o acting_client acting_client.c
```

#### Option 3: Using the VS Code Build Task
Press `Ctrl+Shift+B` to run the default build task that compiles the active file.

### Compilation Flags Explained
- `-lpthread`: Links against the POSIX thread library (required for server)
- `-o <output>`: Specifies the output executable name

## Execution

### Starting the Server

Open a terminal and run:
```bash
./acting_server
```

Expected output:
```
Socket created OK
Bind OK — socket in /tmp/control_led.sock
Listening connections...
```

The server will remain running, waiting for client connections.

### Running the Client

Open another terminal and execute:
```bash
./acting_client <COMMAND>
```

Where `<COMMAND>` is one of:
- `ON` - Turn the LED on
- `OFF` - Turn the LED off
- `STATUS` - Query the current LED state

## Testing

### Test Case 1: Basic ON Command
```bash
./acting_client ON
```
Expected client output:
```
Socket created OK
Connected to the server!
Comando send: 'ON'
Server response: 'LED_OK: ON'
```

Expected server output:
```
[Main] Client connected! Beggining thread...
[Hilo] Received command: 'ON'
[Hilo] State: 1 — Response: 'LED_OK: ON'
[Hilo] Attended client, thread terminated
```

### Test Case 2: Basic OFF Command
```bash
./acting_client OFF
```
Expected client output:
```
Socket created OK
Connected to the server!
Comando send: 'OFF'
Server response: 'LED_OK: OFF'
```

### Test Case 3: STATUS Query
```bash
./acting_client STATUS
```
Expected client output (after ON was set):
```
Socket created OK
Connected to the server!
Comando send: 'STATUS'
Server response: 'LED_STATUS: ON'
```

### Test Case 4: Invalid Command
```bash
./acting_client INVALID
```
Expected client output:
```
Socket created OK
Connected to the server!
Comando send: 'INVALID'
Server response: 'ERROR: INVALID_COMMAND'
```

### Test Case 5: Multiple Concurrent Clients
In separate terminals, run multiple client instances simultaneously:
```bash
# Terminal 1
./acting_client ON

# Terminal 2
./acting_client STATUS

# Terminal 3
./acting_client OFF

# Terminal 4
./acting_client STATUS
```

The server should handle all requests concurrently. Expected sequence:
```
[Main] Client connected! Beggining thread...
[Main] Client connected! Beggining thread...
[Main] Client connected! Beggining thread...
[Main] Client connected! Beggining thread...
[Hilo] Received command: 'ON'
[Hilo] Received command: 'STATUS'
[Hilo] Received command: 'OFF'
[Hilo] Received command: 'STATUS'
[Hilo] State: 1 — Response: 'LED_OK: ON'
[Hilo] State: 1 — Response: 'LED_STATUS: ON'
[Hilo] State: 0 — Response: 'LED_OK: OFF'
[Hilo] State: 0 — Response: 'LED_STATUS: OFF'
```

### Test Case 6: Server Graceful Shutdown
While the server is running, press `CTRL+C`:
```bash
^C
```

Expected output:
```
Listening connections...
^C
```

The server should cleanly shut down, removing the socket file.

### Test Case 7: Client Without Argument (Error Handling)
```bash
./acting_client
```
Expected output:
```
Use: ./client <ON|OFF|STATUS>
```

### Test Case 8: Connection Failure (Server Not Running)
If the server is not running:
```bash
./acting_client ON
```
Expected output:
```
Socket created OK
Error connecting to the server: No such file or directory
```

## Advanced Testing

### Thread Safety Verification
Run multiple clients in rapid succession to verify mutex protection:
```bash
# Run these commands in quick succession
for i in {1..5}; do ./acting_client STATUS & done
```

The server should handle all requests without race conditions or data corruption.

### Socket Cleanup Verification
After stopping the server, verify the socket file is removed:
```bash
ls -la /tmp/control_led.sock
```
Expected: File should not exist (no output or "No such file or directory")

### Memory Leak Check (Optional)
Compile with debug flags and use memory analysis tools:
```bash
gcc-12 -g -o acting_server acting_server.c -lpthread
valgrind ./acting_server &
# Run clients and then press Ctrl+C
```

## Architecture Details

### Communication Flow

1. **Server Initialization**
   - Creates Unix domain socket
   - Binds socket to `/tmp/control_led.sock`
   - Enters listening state

2. **Client Connection**
   - Creates Unix domain socket
   - Connects to server socket
   - Sends command

3. **Command Processing**
   - Server accepts connection
   - Creates new thread for client
   - Thread reads command
   - Thread acquires mutex lock
   - Modifies LED state
   - Thread releases mutex lock
   - Thread sends response
   - Thread terminates

4. **Response**
   - Client receives response
   - Client closes connection

### Thread Safety
- **Shared Resource**: `led_state` variable
- **Synchronization**: POSIX `pthread_mutex_t`
- **Strategy**: Lock-based mutual exclusion

## Notes

- The socket file is created at `/tmp/control_led.sock`
- GPIO operations are commented out (uses pigpio library) - uncomment if running on Raspberry Pi
- Each client connection spawns a detached thread
- The server can handle multiple concurrent clients efficiently

## Future Enhancements

1. Add actual GPIO control with pigpio library
2. Implement persistent state logging
3. Add authentication/authorization
4. Implement timeout for idle clients
5. Add performance metrics and monitoring
6. Support for a commands queue with priority levels

## References

- POSIX Threads Documentation
- Unix Domain Sockets Programming
- Linux IPC (Inter-Process Communication)
- POSIX Signal Handling
