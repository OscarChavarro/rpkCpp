#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
_Polygon2DContour::_Polygon2DContour() : exteriorContour(nullptr), fleetingFlag(false) {}

void _Polygon2DContour::addVertex(double x, double y, double r, double g, double b) { vertices.add(Vertex2D(x, y, r, g, b)); }
void _Polygon2DContour::addVertex(double x, double y) { vertices.add(Vertex2D(x, y)); }
void _Polygon2DContour::pushVertex(double x, double y) { vertices.add(0L, Vertex2D(x, y)); }

double* _Polygon2DContour::getMinMax()
{
    double* minMax = new double[6];
    int size = (int)vertices.size();
    double minX, minY, maxX, maxY;

    if (size > 0) {
        minX = maxX = vertices[0].x;
        minY = maxY = vertices[0].y;
    }
    else {
        minX = minY = 1e308;
        maxX = maxY = -1e308;
    }

    for (int i = 1; i < size; i++) {
        const Vertex2D& v = vertices[i];
        if (v.x > maxX) maxX = v.x;
        if (v.x < minX) minX = v.x;
        if (v.y > maxY) maxY = v.y;
        if (v.y < minY) minY = v.y;
    }

    minMax[0] = minX; minMax[1] = minY; minMax[2] = 0;
    minMax[3] = maxX; minMax[4] = maxY; minMax[5] = 0;
    return minMax;
}

double _Polygon2DContour::calcMinMaxArea(bool)
{
    double* minMax = getMinMax();
    double area = (minMax[3]-minMax[0])*(minMax[4]-minMax[1]);
    delete[] minMax;
    return area;
}

_Polygon2DContour* _Polygon2DContour::getExteriorContour() { return exteriorContour; }
void _Polygon2DContour::setExteriorContour(_Polygon2DContour* contour) { exteriorContour = contour; }

int _Polygon2DContour::compareTo(_Polygon2DContour* obj)
{
    double area = calcMinMaxArea(false);
    double otherArea = obj->calcMinMaxArea(false);
    if (area == otherArea) return 0;
    if (area < otherArea) return -1;
    return 1;
}
