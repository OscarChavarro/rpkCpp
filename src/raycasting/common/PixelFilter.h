#ifndef __PIXEL_FILTER__
#define __PIXEL_FILTER__

class PixelFilter {
  public:
    PixelFilter();
    virtual ~PixelFilter();

    virtual void sample(double *xi1, double *xi2);
};

#endif
