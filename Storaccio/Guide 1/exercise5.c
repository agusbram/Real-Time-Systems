#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>

/**
 * Shows actual time in a digital format (HH-MM-SS)
 * 
 */
void clock_time();

/**
 * Shows cronometer in a format (HH-MM-SS)
 */
void cronometer();

int main(int argc, char const *argv[]) {
    int option = 0;

    /**
     * Prints in console input to do by user, then reads it
     */
    printf("Do you want actual time or a cronometer?\n1. Actual time\n0. Cronometer\n");
    scanf("%d", &option);
    
    /**
     * 
     * If user selects option 1 -> Shows actual time
     * If user selects option 2 -> Shows a cronometer
     */
    if(option) {
        clock_time();
    } else {
        cronometer();
    }
    return 0;
}


void cronometer() {
    /**
     * Timestamps variables
     * @param timespec contains two fields: tv_sec -> seconds passed from 1970, tv_nsec -> additionals nanoseconds
     * @param start: time where cronometer begins
     * @param actual: actual time of every iteration of the loop
     */
    struct timespec start, actual;

    /**
     * This is necessary to print the first second
     */
    long last_second = -1;

    /**
     * Function that returns beginning time of a monotonic clock
     * @param CLOCK_MONOTONIC measures time elapsed where the system was on, and it's not affected by any change of the clock system
     * @param start variable that store time discussed in the previous parameter
     * @returns beggining time
     */
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        /**
        * Function that returns actual time of a monotonic clock
        * @param CLOCK_MONOTONIC measures time elapsed where the system was on, and it's not affected by any change of the clock system
        * @param start variable that store time discussed in the previous parameter
        * @returns actual time
        */
        clock_gettime(CLOCK_MONOTONIC, &actual);

        /**
         * Calculates elapsed time in seconds, substracting actual seconds and initial seconds
         * 
         */
        long seconds = actual.tv_sec - start.tv_sec;

        /**
         * This condition assures us that cronometer will refresh only when a second has passed
         */
        if (seconds != last_second) {
            long minutes = seconds / 60; 
            long hours = minutes / 60;   
            long remaining_seconds = seconds % 60; 
            long remaining_minutes = minutes % 60;

            printf("\rTime: %02ld:%02ld:%02ld (hh:mm:ss)", hours, remaining_minutes, remaining_seconds);

            // This is necessary to quick printing the time. It removes data from output
            fflush(stdout);

            // This is necessary to detect a change of a second
            last_second = seconds;
        }
    }
}

void clock_time() {
    /**
     * @param tm: breaks down all the time in legible components (hours, minutes, seconds, days, months, ...)
     * @param info it will be used to store hours, minutes and seconds
     */
    struct tm *info;

    /**
     * @param time_t: integer that stores seconds elapsed from Epoch. Its useful to measure absolute time or calculate differences in time
     * @param now initializes actual time of the system
     */
    time_t now = time(NULL);

    /**
     * Converts actual stored time (now) to a tm structure that contains the broken-down time
     */
    info = localtime(&now);

    /**
     * Shows the actual time of the structure that contains actual time
     */
    printf("Hora: %02d:%02d:%02d\n",
        info->tm_hour,
        info->tm_min,
        info->tm_sec);
}