#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define QUEUE_LENGTH     10
#define QUEUE_TIMEOUT_MS 10
#define PRODUCER_DELAY_MS 500
#define SENSOR_MAX      100
#define PROCESSING_DELAY_MS 1000  /* simulated consumer processing time */
#define STATS_PRINT_INTERVAL_MS 5000  /* print statistics every 5 seconds */

/* LED GPIO pin */
#define LED_PIN  GPIO_NUM_2

static const char *TAG = "TP_QUEUES";

/* Statistics structure */
typedef struct {
    uint32_t sent_count;
    uint32_t discarded_count;
} statistics_t;

static statistics_t stats = {0, 0};
static QueueHandle_t xDataQueue = NULL;

/*
 * Configure GPIO pin for LED as output.
 * Single LED behavior:
 *   - ON: data was sent to queue (producer active)
 *   - OFF: consumer finished processing the data
 */
static void configure_led(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);  /* start off */

    ESP_LOGI(TAG, "LED configured on GPIO %d", LED_PIN);
}

/*
 * Producer task — pinned to Core 1, priority 2.
 * Every 500 ms it simulates an analog sensor reading (random 0-100)
 * and sends the value through the queue. When data is successfully sent,
 * the LED turns ON solid (stays on until consumer finishes processing).
 * If the queue is full, it waits up to 10 ms before discarding the data.
 */
void vProducerTask(void *pvParameters)
{
    int sensorValue;
    TickType_t xTicksToWait = pdMS_TO_TICKS(QUEUE_TIMEOUT_MS);

    ESP_LOGI(TAG, "Producer task started on Core %d", xPortGetCoreID());

    while (1)
    {
        sensorValue = rand() % (SENSOR_MAX + 1);

        if (xQueueSend(xDataQueue, &sensorValue, xTicksToWait) == pdTRUE)
        {
            stats.sent_count++;
            gpio_set_level(LED_PIN, 1);  /* turn on LED: data in queue */
            ESP_LOGI(TAG, "[PRODUCER] Sent: %d (Total sent: %lu)", 
                     sensorValue, stats.sent_count);
        }
        else
        {
            stats.discarded_count++;
            ESP_LOGE(TAG, "[PRODUCER] Queue full, discarded: %d (Total discarded: %lu)",
                     sensorValue, stats.discarded_count);
        }

        vTaskDelay(pdMS_TO_TICKS(PRODUCER_DELAY_MS));
    }
}

/*
 * Consumer task — pinned to Core 0, priority 1.
 * Stays blocked (idle) until data arrives in the queue.
 * When it receives a value, it:
 *   1. Prints the received value and the core it runs on
 *   2. Simulates work with a delay
 *   3. Turns off the LED after finishing processing
 */
void vConsumerTask(void *pvParameters)
{
    int receivedValue;

    ESP_LOGI(TAG, "Consumer task started on Core %d", xPortGetCoreID());

    while (1)
    {
        if (xQueueReceive(xDataQueue, &receivedValue, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "[CONSUMER] Received: %d | Core: %d | Processing...",
                     receivedValue, xPortGetCoreID());

            /* Simulate processing workload */
            vTaskDelay(pdMS_TO_TICKS(PROCESSING_DELAY_MS));

            /* Turn off LED after processing is done */
            gpio_set_level(LED_PIN, 0);

            ESP_LOGI(TAG, "[CONSUMER] Finished processing value: %d", receivedValue);
        }
    }
}

/*
 * Statistics task — reports data transmission metrics every 5 seconds.
 * Runs on Core 1 with low priority.
 * Displays total sent, total discarded, and success rate.
 */
void vStatisticsTask(void *pvParameters)
{
    ESP_LOGI(TAG, "Statistics task started");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(STATS_PRINT_INTERVAL_MS));

        uint32_t total = stats.sent_count + stats.discarded_count;
        float success_rate = (total > 0) ? (100.0f * stats.sent_count / total) : 0.0f;

        ESP_LOGI(TAG, "========== STATISTICS ==========");
        ESP_LOGI(TAG, "Total Sent: %lu", stats.sent_count);
        ESP_LOGI(TAG, "Total Discarded: %lu", stats.discarded_count);
        ESP_LOGI(TAG, "Success Rate: %.1f%%", success_rate);
        ESP_LOGI(TAG, "================================");
    }
}

/*
 * Entry point (ESP-IDF equivalent of main).
 * Initializes:
 *   1. GPIO (LED)
 *   2. Queue for inter-task communication
 *   3. All three tasks pinned to cores with specific priorities
 */
void app_main(void)
{
    ESP_LOGI(TAG, "System starting...");

    /* Configure LED */
    configure_led();

    /* Create the queue */
    xDataQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int));
    if (xDataQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }
    ESP_LOGI(TAG, "Queue created successfully with capacity: %d", QUEUE_LENGTH);

    /* Create producer task — Core 1, priority 2 */
    xTaskCreatePinnedToCore(
        vProducerTask,
        "Producer",
        2048,
        NULL,
        2,
        NULL,
        1);

    /* Create consumer task — Core 0, priority 1 */
    xTaskCreatePinnedToCore(
        vConsumerTask,
        "Consumer",
        2048,
        NULL,
        1,
        NULL,
        0);

    /* Create statistics task — Core 1, priority 0 (lowest) */
    xTaskCreatePinnedToCore(
        vStatisticsTask,
        "Statistics",
        2048,
        NULL,
        0,
        NULL,
        1);

    ESP_LOGI(TAG, "All tasks created and running");
}
