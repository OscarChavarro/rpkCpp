#ifndef PIXEL_FILTER__
#define PIXEL_FILTER__

class PixelFilter {
  public:
    PixelFilter();
    virtual ~PixelFilter();

    virtual void sample(double *xi1, double *xi2) = 0;
};

#endif
