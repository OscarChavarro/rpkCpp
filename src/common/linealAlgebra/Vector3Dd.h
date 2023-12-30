#ifndef __Vector3Dd__
#define __Vector3Dd__

/* double floating point vectors: The extra precision is sometimes needed eg
 * for sampling a spherical triangle or computing an analytical point to
 * patch factor, also for accurately computing normals and areas ... */
class Vector3Dd {
  public:
    double x;
    double y;
    double z;

    Vector3Dd() {
        x = 0.0;
        y = 0.0;
        z = 0.0;
    }

    Vector3Dd(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
};

#endif
