#ifndef __VERTEX_2_D__
#define __VERTEX_2_D__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
class Vertex2D
{
public:
    double x;
    double y;
    ColorRgb color;

    Vertex2D() : x(0), y(0) {}
    Vertex2D(double x, double y);
    Vertex2D(double x, double y, double r, double g, double b);

    java::String toString() const;
};

#endif
