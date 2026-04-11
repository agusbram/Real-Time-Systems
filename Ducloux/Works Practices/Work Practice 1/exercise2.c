#include <stdio.h> // To use scanf & printf
#include <float.h> // To use limits values of float
#include <limits.h> // To use limits values of signed short

int main(int argc, char const *argv[]) {
    // To verify if number is outside range of signed short
    int number_inserted = 0;

    // [-3.4 * 10³⁸, 3.4 * 10³⁸]
    float number_converted = 0;

    // [-32768, 32767] = signed short
    signed short number_to_convert = 0;

    // Insert number to verify is outside signed short
    printf("Insert your signed short number to convert\n");
    scanf("%d", &number_inserted);

    // Converts maximum & minimum values that are outside range.
    if(number_inserted > SHRT_MAX) {
        printf("Limit maximun passed (32767)");
        return(1);
    } else if(number_inserted < SHRT_MIN) {
        printf("Minimum limit passed (-32768)");
        return(1);
    }

    // If passed verification ==> Convert number to signed short
    number_to_convert = number_inserted;

    // Print number to convert float
    printf("Number to convert: %hd\n", number_to_convert);

    // Conversion is done here
    number_converted = (float)number_to_convert;

    // Printf number converted to signed short
    printf("Number converted: %f\n", number_converted);

    return 0;
}


