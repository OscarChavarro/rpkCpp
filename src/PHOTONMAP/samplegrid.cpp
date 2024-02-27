#include <cstdio>

#include "common/linealAlgebra/Float.h"
#include "PHOTONMAP/samplegrid.h"
#include "PHOTONMAP/discretesampling.h"

CSampleGrid2D::CSampleGrid2D(int xsections, int ysections) {
    m_xsections = xsections;
    m_ysections = ysections;

    m_values = new double[m_xsections * m_ysections];
    m_ysums = new double[m_xsections];

    Init();
}

void CSampleGrid2D::Init() {
    int i, j, index;

    index = 0;

    for ( i = 0; i < m_xsections; i++ ) {
        m_ysums[i] = 0.0;
        for ( j = 0; j < m_ysections; j++ ) {
            m_values[index++] = 0.0;
        }
    }

    m_totalSum = 0.0;
}


void CSampleGrid2D::Print() {
    int i, j, index;
    double ysum, totalsum;

    index = 0;
    printf("Grid size %i %i\n", m_xsections, m_ysections);

    totalsum = 0.0;

    for ( i = 0; i < m_xsections; i++ ) {
        ysum = 0;
        for ( j = 0; j < m_ysections; j++ ) {
            printf("  Val %i %i, %g\n", i, j, m_values[index]);
            ysum += m_values[index];
            totalsum += m_values[index];
            index++;
        }
        printf("Sum %i, %g, summed %g\n", i, m_ysums[i], ysum);
    }

    printf("Total sum: %g, summed %g\n", m_totalSum, totalsum);
}

void CSampleGrid2D::Add(double x, double y, double value) {
    // Precondition: 0 <= x < 1 en 0 <= y < 1

    int xindex, yindex;

    xindex = (int) (x * m_xsections);
    yindex = (int) (y * m_ysections);

    if ( xindex == m_xsections ) {
        xindex--;
    }  // x or y seem to be able to be 1
    if ( yindex == m_ysections ) {
        yindex--;
    }  // x or y seem to be able to be 1

    m_values[ValIndex(xindex, yindex)] += value;
    m_ysums[xindex] += value;
    m_totalSum += value;
}

void CSampleGrid2D::EnsureNonZeroEntries() {
    int index, i, j;
    // Add 3% of the average value to empty grid elements
    double fraction = 0.03 * m_totalSum / (m_xsections * m_ysections);
    double treshold = 1e-10 * m_totalSum;

    index = 0; // ! index is correlated with i,j in for loops

    for ( i = 0; i < m_xsections; i++ ) {
        for ( j = 0; j < m_ysections; j++ ) {
            if ( m_values[index] < treshold ) {
                m_values[index] += fraction;
                m_ysums[i] += fraction;
                m_totalSum += fraction;
            }
            index++;
        }
    }
}

void CSampleGrid2D::Sample(double *x, double *y, double *pdf) {
    int xindex, yindex;
    double xpdf, ypdf;

    if ( m_totalSum < EPSILON ) {
        // No significant data in table, use uniform sampling
        *pdf = 1.0;
        return;
    }

    // Choose x row

    xindex = DSampleDiscrete(m_ysums, m_totalSum, x, &xpdf);

    // Choose y column

    yindex = DSampleDiscrete(m_values + xindex * m_ysections, m_ysums[xindex], y, &ypdf);

    *pdf = xpdf * ypdf;

    // Rescale: x and y are in [0,1[ now we need to sample
    // gridelement (xindex, yindex) uniformly

    double range;

    range = 1.0 / m_xsections;
    *x = (*x + xindex) * range;
    *pdf /= range;

    range = 1.0 / m_ysections;
    *y = (*y + yindex) * range;
    *pdf /= range;  // Uniform sampling: pdf = 1/A(xi,yi) * p(xi) * p(yi)
}

