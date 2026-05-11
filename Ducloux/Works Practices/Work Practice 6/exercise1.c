// gcc -o ejemplo_spin ejemplo_spin.c -lpthread

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
Mutex → If the resource is busy, the thread is blocked (sleep).
Spinlock → The thread remains in a busy loop (waiting) until the resource is released. It is more efficient in very short critical sections.

- A mutex blocks the thread if the resource is unavailable.
- A spinlock keeps the thread actively executing until the resource is released.

Spinlocks are more efficient when:
- The critical section is very short.
- They are used in high-performance or real-time systems.
- They are not recommended if the wait can be long (it consumes CPU).
*/

// Spinlock object
pthread_spinlock_t spin;

void* thread_body_1(void* arg) {
    int* shared_var_ptr = (int*)arg;

    // Critical section
    pthread_spin_lock(&spin);

    (*shared_var_ptr)++;
    printf("%d\n", *shared_var_ptr);

    pthread_spin_unlock(&spin);

    return NULL;
}

void* thread_body_2(void* arg) {
    int* shared_var_ptr = (int*)arg;

    // Critical section
    pthread_spin_lock(&spin);

    *shared_var_ptr += 2;
    printf("%d\n", *shared_var_ptr);

    pthread_spin_unlock(&spin);

    return NULL;
}

int main() {
    int shared_var = 0;

    pthread_t thread1, thread2;

    // Initialize spinlock
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

    int result1 = pthread_create(&thread1, NULL, thread_body_1, &shared_var);
    int result2 = pthread_create(&thread2, NULL, thread_body_2, &shared_var);

    if (result1 || result2) {
        printf("The threads could not be created.\n");
        exit(1);
    }

    result1 = pthread_join(thread1, NULL);
    result2 = pthread_join(thread2, NULL);

    if (result1 || result2) {
        printf("The threads could not be joined.\n");
        exit(2);
    }

    // Destroy spinlock
    pthread_spin_destroy(&spin);

    return 0;
}