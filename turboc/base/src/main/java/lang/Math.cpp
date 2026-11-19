#include <string.h>
#include "java/lang/Math.h"

const double Math::E = 2.7182818284590452354;
const double Math::PI = 3.14159265358979323846;

double
Math::nextAfter(double start, double direction)
{
    if ( start != start || direction != direction ) {
        return start + direction;
    }
    if ( start == direction ) {
        return direction;
    }
    unsigned char bytes[8];
    int i;
    if ( start == 0.0 ) {
        // Smallest subnormal, with the sign of the direction
        for ( i = 0; i < 8; i++ ) {
            bytes[i] = 0;
        }
        bytes[0] = 1;
        if ( direction < 0.0 ) {
            bytes[7] = 0x80;
        }
        double result;
        memcpy(&result, bytes, sizeof(result));
        return result;
    }
    memcpy(bytes, &start, sizeof(bytes));
    // The magnitude grows when moving away from zero
    if ( (start > 0.0) == (direction > start) ) {
        for ( i = 0; i < 8; i++ ) {
            bytes[i] = (unsigned char)(bytes[i] + 1);
            if ( bytes[i] != 0 ) {
                break;
            }
        }
    }
    else {
        for ( i = 0; i < 8; i++ ) {
            unsigned char previous = bytes[i];
            bytes[i] = (unsigned char)(bytes[i] - 1);
            if ( previous != 0 ) {
                break;
            }
        }
    }
    double next;
    memcpy(&next, bytes, sizeof(next));
    return next;
}
