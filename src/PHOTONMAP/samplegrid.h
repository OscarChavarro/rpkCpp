/**
Class for doing multidimensional discrete sampling.
Grid values are doubles. Grid domain is [0,1]^dim
*/

#ifndef __SAMPLE_GRID__
#define __SAMPLE_GRID__

class CSampleGrid2D {
  protected:
    int m_xsections;
    int m_ysections;
    double *m_values;
    double *m_ysums;  // Sum of y columns for faster sampling
    double m_totalSum; // Normalisation factor

    inline int ValIndex(int i, int j) const { return i * m_ysections + j; }

  public:
    CSampleGrid2D(int xsections, int ysections);

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
    void Print();

};

#endif
