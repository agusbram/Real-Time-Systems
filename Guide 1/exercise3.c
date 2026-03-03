#include <stdio.h>

#define SIZE 100

/**
 * 
 * Sorts an array of integers in ascending order using the bubble sort algorithm
 * @param arr Array of integers to be sorted
 * @param size Size of the array to be sorted
 */
void bubble_sort(int arr[], int size);

int main() {
    int numbers[SIZE];
    int n, i;

    // Insert numbers by keyboard
    printf("How many numbers do you want to insert?: ");
    scanf("%d", &n);

    // Reads and stores numbers inserted 
    for (i = 0; i < n; i++) {
        printf("Numero %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    // Prints numbers inserted previously
    printf("\nNumeros ingresados:\n");
    for (i = 0; i < n; i++) {
        printf("%d ", numbers[i]);
    }

    // Sorts numbers inserted in ascending order
    bubble_sort(numbers, n);

    // Prints ascending sorted array
    printf("\nThe sorted (ascending) array is:\n");
    for(int i = 0; i < n; i++) {
        printf("%d ", numbers[i]);
    }

    return 0;
}

void bubble_sort(int arr[], int size) {
    int temp;

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - 1 - i; j++) {

            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}