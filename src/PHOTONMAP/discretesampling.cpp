#include "PHOTONMAP/discretesampling.h"

int
discreteSample(
    const double probabilities[],
    double total,
    double *x1,
    double *probabilityDensityFunction)
{
    int i = 0;
    double sum;
    double left;
    double sample = *x1 * total;

    sum = probabilities[0];

    while ( sample > sum ) {
        i++;
        sum += probabilities[i];
    }

    // Rescale x_1
    left = sum - probabilities[i];

    *x1 = ((sample - left) / (sum - left));
    *probabilityDensityFunction = probabilities[i] / total;
    return i;
}
