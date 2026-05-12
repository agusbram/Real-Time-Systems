// gcc -o ejemplo5 ejemplo5.c -lpthread

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h> // For errno and strerror function
// The POSIX standard header for using pthread library
#include <pthread.h>
// Semaphores are not exposed through pthread.h
#include <semaphore.h>

// A barrier to make hydrogen and oxygen threads synchronized
pthread_barrier_t water_barrier;

// A mutex in order to synchronize oxygen threads
pthread_mutex_t oxygen_mutex;

// A general semaphore to make hydrogen threads synchronized
sem_t* hydrogen_sem;

// A shared integer counting the number of made water molecules
unsigned int num_of_water_molecules;

void* hydrogen_thread_body(void* arg) {
	// Two hydrogen threads can enter this critical section
	sem_wait(hydrogen_sem);
	// Wait for the other hydrogen thread to join
	pthread_barrier_wait(&water_barrier);
	sem_post(hydrogen_sem);
	return NULL;
}

void* oxygen_thread_body(void* arg) {
	pthread_mutex_lock(&oxygen_mutex);
	// Wait for the hydrogen threads to join
	pthread_barrier_wait(&water_barrier);
	num_of_water_molecules++;
	pthread_mutex_unlock(&oxygen_mutex);
	return NULL;
}

int main(int argc, char** argv) {
	num_of_water_molecules = 0;
	
	// Initialize oxygen mutex
	pthread_mutex_init(&oxygen_mutex, NULL);
	
	// Initialize hydrogen semaphore
	sem_t local_sem;
	hydrogen_sem = &local_sem;
	sem_init(hydrogen_sem, 0, 2);

	// Initialize water barrier
	pthread_barrier_init(&water_barrier, NULL, 3);

	// For creating 50 water molecules, we need 50 oxygen atoms and
	// 100 hydrogen atoms
	pthread_t thread[150];
	
	// Create oxygen threads
	for (int i = 0; i < 50; i++) {
		if (pthread_create(thread + i, NULL, oxygen_thread_body, NULL)) {
			printf("Couldn't create an oxygen thread.\n");
			exit(1);
		}
	}
	
	// Create hydrogen threads
	for (int i = 50; i < 150; i++) {
		if (pthread_create(thread + i, NULL, hydrogen_thread_body, NULL)) {
			printf("Couldn't create an hydrogen thread.\n");
			exit(2);
		}
	}
	
	printf("Waiting for hydrogen and oxygen atoms to react ...\n");
	
	// Wait for all threads to finish
	for (int i = 0; i < 150; i++) {
		if (pthread_join(thread[i], NULL)) {
			printf("The thread could not be joined.\n");
			exit(3);
		}
	}
	
	printf("Number of made water molecules: %d\n", num_of_water_molecules);
	
	sem_destroy(hydrogen_sem);

	return 0;
}
