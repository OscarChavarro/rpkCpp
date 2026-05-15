#ifndef __POLYGON_VERTEX__
#define __POLYGON_VERTEX__

#include "common/VSDK.h"

class PolygonVertex {
  public:
    double sx; // Screen space position (sometimes homo)
    double sy;
    double sz;
    double sw;
    double x; // World space position
    double y;
    double z;
    double u; // Texture position (sometimes homogeneous)
    double v;
    double r; // (red,green,blue) color
    double g;
    double b;

    inline double
    getCoord(int i) const {
        switch ( i ) {
            case 0:
                return sx;
            case 1:
                return sy;
            case 2:
                return sz;
            case 3:
                return sw;
            case 4:
                return x;
            case 5:
                return y;
            case 6:
                return z;
            case 7:
                return u;
            case 8:
                return v;
            case 9:
                return r;
            case 10:
                return g;
            case 11:
                return b;
            default:
                return 0.0;
        }
    }
};

#endif
