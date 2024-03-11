/**
Class for doing multidimensional discrete sampling.
Grid values are doubles. Grid domain is [0,1]^dim
*/

#ifndef __SAMPLE_GRID__
#define __SAMPLE_GRID__

class CSampleGrid2D {
  protected:
    int xSections;
    int ySections;
    double *values;
    double *ySums;  // Sum of y columns for faster sampling
    double totalSum; // Normalisation factor

    inline int ValIndex(int i, int j) const {
        return i * ySections + j;
    }

  public:
    CSampleGrid2D(int xSectionsParam, int ySectionsParam);

    void Init(); // Reinitialise, keeping current number of sections

    // Add a contribution to a certain grid element
    void Add(double x, double y, double value);

    // Ensure there are no zero value entries.
    // A small percentage of the totalPower is added to empty grid elements

    void EnsureNonZeroEntries();

    // Sample a point in a grid element. 'Random' values x and y get rescaled
    // so that they sample a uniform point in the selected grid element.
    // Note that their range is not [0,1] anymore, but smaller
    // pdf for sampling this point is filled in.
    void Sample(double *x, double *y, double *pdf);
};

#endif
