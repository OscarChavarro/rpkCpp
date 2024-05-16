#include "common/linealAlgebra/Float.h"
#include "PHOTONMAP/samplegrid.h"
#include "PHOTONMAP/discretesampling.h"

CSampleGrid2D::CSampleGrid2D(int xSectionsParam, int ySectionsParam): totalSum() {
    xSections = xSectionsParam;
    ySections = ySectionsParam;

    values = new double[xSections * ySections];
    ySums = new double[xSections];

    Init();
}

void CSampleGrid2D::Init() {
    int index;

    index = 0;

    for ( int i = 0; i < xSections; i++ ) {
        ySums[i] = 0.0;
        for ( int j = 0; j < ySections; j++ ) {
            values[index++] = 0.0;
        }
    }

    totalSum = 0.0;
}

void
CSampleGrid2D::Add(double x, double y, double value) {
    // Precondition: 0 <= x < 1 en 0 <= y < 1

    int xIndex;
    int yIndex;

    xIndex = (int) (x * xSections);
    yIndex = (int) (y * ySections);

    if ( xIndex == xSections ) {
        xIndex--;
    }  // x or y seem to be able to be 1
    if ( yIndex == ySections ) {
        yIndex--;
    }  // x or y seem to be able to be 1

    values[ValIndex(xIndex, yIndex)] += value;
    ySums[xIndex] += value;
    totalSum += value;
}

void CSampleGrid2D::EnsureNonZeroEntries() {
    int index;
    // Add 3% of the average value to empty grid elements
    double fraction = 0.03 * totalSum / (xSections * ySections);
    double threshold = 1e-10 * totalSum;

    index = 0; // ! index is correlated with i,j in for loops

    for ( int i = 0; i < xSections; i++ ) {
        for ( int j = 0; j < ySections; j++ ) {
            if ( values[index] < threshold ) {
                values[index] += fraction;
                ySums[i] += fraction;
                totalSum += fraction;
            }
            index++;
        }
    }
}

void
CSampleGrid2D::sample(double *x, double *y, double *probabilityDensityFunction) const {
    int xIndex;
    int yIndex;
    double xPdf;
    double yPdf;

    if ( totalSum < EPSILON ) {
        // No significant data in table, use uniform sampling
        *probabilityDensityFunction = 1.0;
        return;
    }

    // Choose x row
    xIndex = discreteSample(ySums, totalSum, x, &xPdf);

    // Choose y column
    yIndex = discreteSample(values + xIndex * ySections, ySums[xIndex], y, &yPdf);

    *probabilityDensityFunction = xPdf * yPdf;

    // Rescale: x and y are in [0,1[ now we need to sample
    // grid element (xIndex, yIndex) uniformly

    double range;

    range = 1.0 / xSections;
    *x = (*x + xIndex) * range;
    *probabilityDensityFunction /= range;

    range = 1.0 / ySections;
    *y = (*y + yIndex) * range;
    *probabilityDensityFunction /= range;  // Uniform sampling: pdf = 1/A(xi,yi) * p(xi) * p(yi)
}

