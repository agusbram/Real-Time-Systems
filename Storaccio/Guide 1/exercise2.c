#include <stdio.h>
#define SIZE 10000

/**
 * Count the quantity of words by counting the space letter ' '
 * Analizes if the counter it's inside a word or not to count perfectly well
 * @param text Array of chars to count every word
 * @returns quantity of words of the character array
 */
int count_words(char text[]);

int main(int argc, char const *argv[]) {
    // Definition and declaration of an string array
    char words[SIZE] = {0};

    // Insert the words to be counted
    printf("Please insert the words that you want to count\n");
    fgets(words, sizeof(words), stdin);

    // Count the words inserted previously
    int total = count_words(words);

    // Print the quantity of words
    printf("The quantity of words are %d\n", total);
    return 0;
}

int count_words(char text[]) {
    // Definition and declaration of local variables
    int count = 0;
    int inside_word = 0;

    // Iterates through a for loop
    for (int i = 0; text[i] != '\0'; i++) {
        // If doesn't find an endless character step inside of if
        if (text[i] != ' ' && text[i] != '\n' && text[i] != '\t') {
            // If it's not inside the word counted previously (this fixes the error of count multiple times the same word)
            if (!inside_word) {
                count++;
                inside_word = 1; // Set the flag to don't count the same word
            }
        } else {
            // If it's find an endless character it means that it begins a new word
            inside_word = 0;
        }
    }

    return count;
}