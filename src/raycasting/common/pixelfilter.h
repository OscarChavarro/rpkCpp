#ifndef _PIXELFILTER_H_
#define _PIXELFILTER_H_

class pixelFilter {
public:
    pixelFilter();

    virtual ~pixelFilter();

    virtual void sample(double *xi1, double *xi2);
};

class BoxFilter : public pixelFilter {
public:
    BoxFilter();

    ~BoxFilter();

    void sample(double *, double *);
};

class TentFilter : public pixelFilter {
public:
    TentFilter();

    ~TentFilter();

    void sample(double *, double *);
};

class NormalFilter : public pixelFilter {
public:
    explicit NormalFilter(double s = 0.70710678, double d = 2.0);

    ~NormalFilter();

    void sample(double *, double *);

    double sigma;
    double dist;
};

#endif /* _PIXELFILTER_H_ */
