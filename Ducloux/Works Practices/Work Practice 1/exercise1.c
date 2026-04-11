#include <stdio.h> // To use scanf & printf
#include <float.h> // To use limits values of float
#include <limits.h> // To use limits values of signed short

int main(int argc, char const *argv[]) {

    // [-3.4 * 10³⁸, 3.4 * 10³⁸]
    float number_to_convert = 0;

    // [-32768, 32767] = signed short
    // While the float stays within range, the conversion is valid
    // the fractional part is truncated during conversion
    signed short number_converted = 0;

    printf("Insert your float number to convert\n");

    // Verifies if number inserted is a correct float number inside the range mentioned above
    if (scanf("%f", &number_to_convert) != 1) {
        printf("Invalid input\n");
        return 1;
    }

    // Print number to convert float
    printf("Number to convert: %f\n", number_to_convert);

    // Converts maximum & minimum values that are outside range.
    // This is a 'saturation or clamping' of the value inside the range of signed short
    if(number_to_convert > SHRT_MAX) {
        number_to_convert = SHRT_MAX;
    } else if(number_to_convert < SHRT_MIN) {
        number_to_convert = SHRT_MIN;
    }

    // Print number saturated float
    printf("Number saturated: %f\n", number_to_convert);

    number_converted = (signed short)number_to_convert;

    // Printf number converted to signed short
    printf("Number converted: %hd\n", number_converted);

    return 0;
}


