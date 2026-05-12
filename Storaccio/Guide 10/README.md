# Dual-Core Queue Communication System with FreeRTOS on ESP32

## Overview

This project implements a **real-time telemetry system** on the ESP32 microcontroller using FreeRTOS. It demonstrates inter-core communication through message queues, task synchronization, and visual feedback via LED indicators.

### Key Features

- **Dual-core execution**: Producer task runs on Core 1, Consumer task on Core 0
- **Thread-safe communication**: Uses FreeRTOS queues for safe data exchange
- **Real-time monitoring**: LED feedback shows system activity
- **Statistics tracking**: Monitors sent/discarded data and success rate
- **Comprehensive logging**: Uses ESP_LOGI/ESP_LOGE for debugging

---

## System Architecture

### Tasks

| Task | Core | Priority | Function |
|------|------|----------|----------|
| **Producer** | Core 1 | 2 (High) | Generates simulated sensor data every 500ms and sends to queue |
| **Consumer** | Core 0 | 1 (Low) | Receives and processes data, controls LED indicator |
| **Statistics** | Core 1 | 0 (Lowest) | Reports metrics every 5 seconds |

### Communication

- **Queue**: FIFO buffer with capacity of 10 integers
- **Data flow**: Producer → Queue → Consumer
- **Timeout**: Producer waits max 10ms if queue is full, then discards

### Hardware

- **Microcontroller**: ESP32 (dual-core)
- **LED**: GPIO 2 (built-in blue LED)
- **Communication**: UART (USB) for serial monitoring

---

## LED Behavior

The single LED provides visual feedback of system activity:

| State | Meaning |
|-------|---------|
| **ON** | Producer successfully sent data to queue |
| **OFF** | Consumer finished processing the data |

**Timeline Example:**
```
t=0ms    → LED ON (data sent to queue)
t=500ms  → LED ON (another data sent, first still processing)
t=1000ms → LED OFF (first data finished processing)
t=1500ms → LED ON (new data sent)
t=2500ms → LED OFF (previous data finished processing)
```

---

## Getting Started

### Prerequisites

1. **ESP32 Development Board** (e.g., ESP32-WROOM)
2. **ESP-IDF Toolchain** v6.0.1 or later
   - Installation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html
3. **USB Cable** for programming and serial monitoring

### Installation

1. **Clone/Navigate to the project:**
   ```bash
   cd Storaccio/Guide\ 10
   ```

2. **Verify project structure:**
   ```bash
   ls -la
   ```
   Expected output:
   ```
   CMakeLists.txt
   .vscode/
   main/
     ├── CMakeLists.txt
     └── main.c
   U3_lab4.pdf
   README.md
   ```

3. **Set the ESP-IDF environment:**
   ```bash
   source ~/esp/esp-idf/export.sh
   ```
   (Adjust path if your ESP-IDF is installed elsewhere)

---

## Building and Flashing

### Step 1: Build the project
```bash
idf.py build
```

Expected output ends with:
```
[100%] Built target app_process
```

### Step 2: Connect ESP32 via USB

Identify the serial port:
```bash
ls /dev/ttyUSB*
# or
ls /dev/tty.usbserial*  # macOS
```

### Step 3: Flash to ESP32
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your actual port if different.

The `monitor` flag automatically opens the serial monitor to view logs.

### Step 4: View Output

Once flashing completes, you'll see logs like:
```
I (245) TP_QUEUES: LED configured on GPIO 2
I (250) TP_QUEUES: Queue created successfully with capacity: 10
I (500) TP_QUEUES: Producer task started on Core 1
I (505) TP_QUEUES: Consumer task started on Core 0
I (510) TP_QUEUES: Statistics task started
I (515) TP_QUEUES: [PRODUCER] Sent: 42 (Total sent: 1)
I (1515) TP_QUEUES: [CONSUMER] Received: 42 | Core: 0 | Processing...
I (2515) TP_QUEUES: [CONSUMER] Finished processing value: 42
```

---

## Project Files Explained

### `main/main.c` (Main Source Code)

**Key Components:**

1. **`configure_led()`**
   - Initializes GPIO 2 as output
   - Sets initial state to OFF

2. **`vProducerTask()`**
   - Runs every 500ms
   - Generates random sensor value (0-100)
   - Turns LED ON when data is sent
   - Logs success/failure with counters

3. **`vConsumerTask()`**
   - Blocked until data arrives in queue
   - Receives value from queue
   - Simulates 1000ms processing time
   - Turns LED OFF after processing completes

4. **`vStatisticsTask()`**
   - Reports metrics every 5 seconds
   - Calculates and displays success rate

5. **`app_main()`**
   - Entry point (ESP-IDF equivalent of main())
   - Initializes LED, queue, and all tasks

### `CMakeLists.txt` Files

- **Root CMakeLists.txt**: Project configuration
- **main/CMakeLists.txt**: Component registration

### `.vscode/settings.json`

Contains IDE configuration:
- ESP-IDF toolchain path
- Default serial port

---

## Understanding the Code

### Queue Operations

**Sending (Producer):**
```c
if (xQueueSend(xDataQueue, &sensorValue, xTicksToWait) == pdTRUE)
{
    // Success: data added to queue
    gpio_set_level(LED_PIN, 1);
}
else
{
    // Failure: queue full, data dropped
    stats.discarded_count++;
}
```

**Receiving (Consumer):**
```c
if (xQueueReceive(xDataQueue, &receivedValue, portMAX_DELAY) == pdTRUE)
{
    // Data received and stored in receivedValue
    // portMAX_DELAY = wait indefinitely until data available
}
```

### Task Pinning to Cores

```c
xTaskCreatePinnedToCore(
    vProducerTask,     // Task function
    "Producer",        // Task name
    2048,              // Stack size (bytes)
    NULL,              // Parameters
    2,                 // Priority (0=lowest, higher=more priority)
    NULL,              // Task handle
    1);                // Core ID (0 or 1)
```

---

## Monitoring and Debugging

### View Real-time Logs
While monitor is running, you see live output. Exit with `Ctrl+]`

### Connect to Monitor Later
```bash
idf.py -p /dev/ttyUSB0 monitor
```

### Monitor with Timestamp
```bash
idf.py -p /dev/ttyUSB0 monitor --timestamp="[%H:%M:%S.%f]"
```

### View Build Output
```bash
idf.py build -v  # Verbose mode
```

---

## Troubleshooting

### Issue: "idf.py: command not found"
**Solution**: Source the ESP-IDF environment script first:
```bash
source ~/esp/esp-idf/export.sh
```

### Issue: "Port not found" or "Permission denied"
**Solutions**:
1. Check port is connected: `ls /dev/ttyUSB*`
2. Add user to dialout group: `sudo usermod -a -G dialout $USER`
3. Restart terminal or run: `newgrp dialout`

### Issue: "Queue creation failed"
**Solution**: Ensure sufficient heap memory. This rarely happens on ESP32.

### Issue: LED not responding
**Solution**: 
1. Verify GPIO 2 is not used by other peripherals
2. Check LED polarity (GND and signal connections)
3. Inspect logs for GPIO configuration errors

---

## Performance Metrics

With default settings:

| Metric | Value |
|--------|-------|
| Producer frequency | 1 per 500ms |
| Consumer processing time | 1000ms |
| Queue capacity | 10 items |
| Data loss (normal conditions) | 0% |
| System responsiveness | Real-time |

**Expected behavior**: You'll see LED patterns:
- Short ON/OFF cycles initially
- Then settling into: ON 1000ms, OFF 500ms cycle

---

## Customization

### Change Producer Frequency
Edit line 11 in `main/main.c`:
```c
#define PRODUCER_DELAY_MS 500  // Change to desired interval
```

### Change Processing Time
Edit line 13:
```c
#define PROCESSING_DELAY_MS 1000  // Change to desired duration
```

### Change Queue Capacity
Edit line 9:
```c
#define QUEUE_LENGTH 10  // Increase for more buffering
```

### Change LED Pin
Edit line 17:
```c
#define LED_PIN GPIO_NUM_2  // Change to different GPIO
```

After any changes, rebuild:
```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Learning Objectives

This project demonstrates:

✓ **FreeRTOS fundamentals**: Task creation, scheduling, priorities  
✓ **Inter-task communication**: Safe data exchange via queues  
✓ **Dual-core execution**: Pinning tasks to specific CPU cores  
✓ **Real-time systems**: Timing predictability and responsiveness  
✓ **Hardware abstraction**: GPIO control via ESP-IDF drivers  
✓ **Synchronization patterns**: Producer-consumer model  
✓ **Logging best practices**: ESP_LOGI/ESP_LOGE usage  

---

## Course Information

**Subject**: Sistemas de Tiempo Real (Real-Time Systems)  
**Unit**: Unidad 4 (Unit 4)  
**Technology**: ESP32, FreeRTOS, Queue-based Communication  
**Instructor**: Ing. Storaccio Luis  

---

## License

Educational material for Real-Time Systems course at IUA.

---

## Support

For issues or questions:
1. Check the **Troubleshooting** section above
2. Review logs for error messages
3. Verify hardware connections
4. Consult the course materials or instructor

---

**Last Updated**: May 11, 2026  
**Project Version**: 1.0
