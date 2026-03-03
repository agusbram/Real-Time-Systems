/**
 * This function does a sum between two numbers. It prompts
 * the user to enter two numbers and select an operation, then display the result of the chosen operation.
 * @param num1 Number 1 to sum
 * @param num2 Number 2 to sum
 */
void do_sum(float *num1, float *num2);

/**
 * This function does a substract between two numbers. It prompts
 * the user to enter two numbers and select an operation, then display the result of the chosen operation.
 * @param num1 Number 1 to substract
 * @param num2 Number 2 to substract
 */
void do_substract(float *num1, float *num2);

/**
 * This function does a multiplication between two numbers. It prompts
 * the user to enter two numbers and select an operation, then display the result of the chosen operation.
 * @param num1 Number 1 to multiply
 * @param num2 Number 2 to multiply
 */
void do_multiplication(float *num1, float *num2);

/**
 * This function does a division between two numbers. It prompts
 * the user to enter two numbers and select an operation, then display the result of the chosen operation.
 * @param num1 Number 1 to divide
 * @param num2 Number 2 to divide
 */
void do_division(float *num1, float *num2);

int main(int argc, char const *argv[]) {
    // Definition and declaration of variables
    int option = 0;
    float num1 = 0;
    float num2 = 0;

    // Print in console
    printf("Please select an option to do following operations:\n");
    printf("1.Sum\n2.Subtract\n3.Multiply\n4.Divide\n");
    scanf("%d", &option);

    
    // Beginning of switch case
    printf("Please enter two numbers: ");
    scanf("%f %f", &num1, &num2);    
    switch(option) {
        case 1:
            // Sum operation
            do_sum(&num1, &num2);
            break;
        case 2:
            // Substract operation
            do_substract(&num1, &num2);
            break;
        case 3:
            // Multiplication operation
            do_multiplication(&num1, &num2);
            break;
        case 4:
            // Division operation
            do_division(&num1, &num2);
            break;
        default:
            // Incorrect options
            printf("Please insert a number between 1 and 4.");
            break;
    }
    return 0;
}

void do_sum(float *num1, float *num2) {
    printf("The sum of %f and %f is %f\n", *num1, *num2, (*num1 + *num2));
}

void do_substract(float *num1, float *num2) {
    printf("The substract of %f and %f is %f\n", *num1, *num2, (*num1 - *num2));
}

void do_multiplication(float *num1, float *num2) {
    printf("The multiplication of %f and %f is %f\n", *num1, *num2, (*num1 * *num2));
}

void do_division(float *num1, float *num2) {
    if (*num2 == 0) {
        printf("Error: Division by zero is not allowed.\n");
        exit(1);
    } else {
        printf("The division of %f and %f is %f\n", *num1, *num2, (*num1 / *num2));
    }
}