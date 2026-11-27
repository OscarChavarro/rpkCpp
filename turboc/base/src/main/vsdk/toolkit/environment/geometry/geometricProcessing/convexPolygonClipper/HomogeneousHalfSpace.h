#ifndef __HOMOGENEOUS_HALF_SPACE__
#define __HOMOGENEOUS_HALF_SPACE__

/**
Immutable clipping half-space in homogeneous coordinates: the vertices with
a * sx + b * sy + c * sz + d * sw <= 0 are inside, where (a, b, c, d) are the
x, y, z and w coefficients. A convex window is the intersection of such
half-spaces ([SUTH1974b], p. 33). For example, the screen box side x >= xMin
is (-1, 0, 0, xMin) and x <= xMax is (1, 0, 0, -xMax).
*/
class HomogeneousHalfSpace {
  private:
    double xCoefficient;
    double yCoefficient;
    double zCoefficient;
    double wCoefficient;

  public:
    HomogeneousHalfSpace(): xCoefficient(0.0), yCoefficient(0.0), zCoefficient(0.0), wCoefficient(0.0) {}
    HomogeneousHalfSpace(double inXCoefficient, double inYCoefficient, double inZCoefficient, double inWCoefficient):
        xCoefficient(inXCoefficient), yCoefficient(inYCoefficient), zCoefficient(inZCoefficient), wCoefficient(inWCoefficient) {}

    double getXCoefficient() const { return xCoefficient; }
    double getYCoefficient() const { return yCoefficient; }
    double getZCoefficient() const { return zCoefficient; }
    double getWCoefficient() const { return wCoefficient; }
};

#endif
