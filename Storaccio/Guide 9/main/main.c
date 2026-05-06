#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

// ─── Pins ────────────────────────────────────────────────
// GPIO2 is the built-in LED on most ESP32 dev boards.
// GPIO0 is the onboard BOOT button.
#define LED_PIN     GPIO_NUM_2
#define BUTTON_PIN  GPIO_NUM_0

// ─── Debounce ─────────────────────────────────────────────
// Minimum time between two valid button presses (in ms).
// Adjust this value if bouncing persists (try between 150 and 300).
#define DEBOUNCE_MS  200

static const char *TAG = "TP_RTOS";

// ─── Global variables ─────────────────────────────────────
static SemaphoreHandle_t button_semaphore = NULL;  // binary semaphore
static TickType_t last_event_tick = 0;             // tracks time between events
static int led_state = 0;                          // current LED state

// Stores the tick of the LAST interrupt for debounce purposes.
// volatile because it is both written and read inside the ISR.
static volatile TickType_t last_isr_tick = 0;


// ════════════════════════════════════════════════════════════
//  ISR — Interrupt Service Routine with time-based debounce
//  RULES: no printf, no delays, no blocking functions.
//  IRAM_ATTR: forces the code to stay in RAM for faster execution.
// ════════════════════════════════════════════════════════════
static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t higher_priority_woken = pdFALSE;

    // Get the current tick count from within the ISR
    TickType_t now = xTaskGetTickCountFromISR();

    // Calculate how much time has passed since the last interrupt
    TickType_t elapsed = (now - last_isr_tick) * portTICK_PERIOD_MS;

    // Only process the event if at least DEBOUNCE_MS milliseconds have passed
    if (elapsed >= DEBOUNCE_MS) {
        last_isr_tick = now;  // update the reference timestamp
        xSemaphoreGiveFromISR(button_semaphore, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }
    // If not enough time has passed, the interrupt is silently ignored
}


// ════════════════════════════════════════════════════════════
//  Processing Task — Core 1, Priority 3
//  Starts in BLOCKED state on xSemaphoreTake and only runs
//  when the ISR releases the semaphore.
// ════════════════════════════════════════════════════════════
void processing_task(void *pvParameters)
{
    last_event_tick = xTaskGetTickCount();  // initial time reference

    while (1) {
        // portMAX_DELAY = waits indefinitely (no timeout)
        // The task stays BLOCKED here until it receives the semaphore
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) {

            // Calculate elapsed time since the last event
            TickType_t current_tick = xTaskGetTickCount();
            uint32_t elapsed_ms = (current_tick - last_event_tick) * portTICK_PERIOD_MS;
            last_event_tick = current_tick;

            // Toggle the LED
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);

            // Print result (safe here — NOT allowed inside the ISR)
            ESP_LOGI(TAG, "Evento detectado! Time desde el ultimo evento: %lu ms", elapsed_ms);
        }
    }
}


// ════════════════════════════════════════════════════════════
//  Telemetry Task — Core 1, Priority 1
//  Software watchdog: reports every 5 seconds that the system is alive.
// ════════════════════════════════════════════════════════════
void telemetry_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Sistema Operativo Saludable");
        vTaskDelay(pdMS_TO_TICKS(5000));  // blocks for 5 seconds, releases the CPU
    }
}


// ════════════════════════════════════════════════════════════
//  app_main — entry point in ESP-IDF (equivalent to main())
//  Configures hardware, creates the semaphore, and launches tasks.
// ════════════════════════════════════════════════════════════
void app_main(void)
{
    // ── 1. Configure LED as output ────────────────────────
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);  // starts off

    // ── 2. Configure button with interrupt ────────────────
    gpio_config_t io_conf = {
        .pin_bit_mask  = (1ULL << BUTTON_PIN), // bitmask for the selected pin
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = GPIO_PULLUP_ENABLE,   // internal pull-up (idle state = HIGH)
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_NEGEDGE,    // trigger on FALLING edge (HIGH → LOW)
    };
    gpio_config(&io_conf);

    // Install the global ISR service (called only once)
    gpio_install_isr_service(0);

    // Attach the ISR handler to the button pin
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    // ── 3. Create binary semaphore ────────────────────────
    // NOTE: must be created BEFORE the tasks to prevent a task
    // from attempting to take it before it exists
    button_semaphore = xSemaphoreCreateBinary();

    // ── 4. Create tasks ───────────────────────────────────
    xTaskCreatePinnedToCore(
        processing_task,   // task function
        "Processing",      // task name (for debugging)
        2048,              // stack size in bytes
        NULL,              // parameters (unused)
        3,                 // priority (highest in this system)
        NULL,              // handle (not needed)
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        telemetry_task,
        "Telemetry",
        2048,
        NULL,
        1,                 // low priority, does not compete with Processing
        NULL,
        1
    );

    // app_main can return: tasks remain alive independently
}