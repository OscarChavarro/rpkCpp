#include "PHOTONMAP/discretesampling.h"

int
DSampleDiscrete(
    const double probabilities[],
    double total,
    double *x_1,
    double *pdf)
{
    int i = 0;
    double sum, left;
    double sample = *x_1 * total;

    sum = probabilities[0];

    while ( sample > sum ) {
        i++;
        sum += probabilities[i];
    }

    // Rescale x_1
    left = sum - probabilities[i];

    *x_1 = ((sample - left) / (sum - left));
    *pdf = probabilities[i] / total;
    return i;
}
