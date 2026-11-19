/**
Class for doing multidimensional discrete sampling.
Grid values are doubles. Grid domain is [0,1]^dim
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.linealAlgebra.Numeric;

public class SampleGrid2D {
    private int xSections;
    private int ySections;
    private double[] values;
    private double[] ySums;  // Sum of y columns for faster sampling
    private double totalSum; // Normalisation factor

    private int valIndex(int i, int j) {
        return i * ySections + j;
    }

    public SampleGrid2D(int xSectionsParam, int ySectionsParam) {
        totalSum = 0.0;
        xSections = xSectionsParam;
        ySections = ySectionsParam;

        values = new double[xSections * ySections];
        ySums = new double[xSections];

        init();
    }

    public void init() {
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

    // Add a contribution to a certain grid element
    public void add(double x, double y, double value) {
        // Precondition: 0 <= x < 1 en 0 <= y < 1

        int xIndex;
        int yIndex;

        xIndex = (int)(x * xSections);
        yIndex = (int)(y * ySections);

        if ( xIndex == xSections ) {
            xIndex--;
        }  // x or y seem to be able to be 1
        if ( yIndex == ySections ) {
            yIndex--;
        }  // x or y seem to be able to be 1

        values[valIndex(xIndex, yIndex)] += value;
        ySums[xIndex] += value;
        totalSum += value;
    }

    // Ensure there are no zero value entries.
    // A small percentage of the totalPower is added to empty grid elements
    public void EnsureNonZeroEntries() {
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

    // Sample a point in a grid element. 'Random' values x and y get rescaled
    // so that they sample a uniform point in the selected grid element.
    // Note that their range is not [0,1] anymore, but smaller
    // probabilityDensityFunction for sampling this point is filled in.
    public void sample(double[] x, double[] y, double[] probabilityDensityFunction) {
        int xIndex;
        int yIndex;
        double[] xPdf = new double[1];
        double[] yPdf = new double[1];

        if ( totalSum < Numeric.EPSILON ) {
            // No significant data in table, use uniform sampling
            probabilityDensityFunction[0] = 1.0;
            return;
        }

        // Choose x row
        xIndex = DiscreteSampling.sample(ySums, totalSum, x, xPdf);

        // Choose y column
        double[] row = new double[ySections];
        System.arraycopy(values, xIndex * ySections, row, 0, ySections);
        yIndex = DiscreteSampling.sample(row, ySums[xIndex], y, yPdf);

        probabilityDensityFunction[0] = xPdf[0] * yPdf[0];

        // Rescale: x and y are in [0,1[ now we need to sample
        // grid element (xIndex, yIndex) uniformly

        double range;

        range = 1.0 / xSections;
        x[0] = (x[0] + xIndex) * range;
        probabilityDensityFunction[0] /= range;

        range = 1.0 / ySections;
        y[0] = (y[0] + yIndex) * range;
        probabilityDensityFunction[0] /= range;  // Uniform sampling: pdf = 1/A(xi,yi) * p(xi) * p(yi)
    }
}
