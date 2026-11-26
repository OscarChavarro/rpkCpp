#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
Vertex2D::Vertex2D(double inX, double inY) : x(inX), y(inY), color() {}
Vertex2D::Vertex2D(double inX, double inY, double r, double g, double b) : x(inX), y(inY), color(r, g, b) {}

java::String Vertex2D::toString() const
{
    return "<" + VSDK::formatDouble(x) + ", " + VSDK::formatDouble(y) + ">";
}
