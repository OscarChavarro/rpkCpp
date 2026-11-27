#ifndef __POLYGON_2_D_CONTOUR__
#define __POLYGON_2_D_CONTOUR__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
class _Polygon2DContour
{
public:
    ArrayList<Vertex2D> vertices;
    // If this contour is a hole, exteriorContour is the contour that contains it.
private:
    _Polygon2DContour* exteriorContour;
public:
    BOOLEAN fleetingFlag; //Caution, not a long term flag.

    _Polygon2DContour();

    void addVertex(double x, double y, double r, double g, double b);
    void addVertex(double x, double y);
    void pushVertex(double x, double y);

    double* getMinMax();
    double calcMinMaxArea(BOOLEAN modifyState);

    _Polygon2DContour* getExteriorContour();
    void setExteriorContour(_Polygon2DContour* exteriorContour);

    int compareTo(_Polygon2DContour* obj);
};

#endif
