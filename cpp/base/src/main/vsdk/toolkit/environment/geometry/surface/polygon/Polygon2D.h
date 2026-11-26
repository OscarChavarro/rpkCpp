#ifndef __POLYGON_2_D__
#define __POLYGON_2_D__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class _Polygon2DContour;
class Vertex2D;
class Ray;
class RayHit;
template <class T> class BinaryTreeNode;

class Polygon2D : public Surface
{
public:
    java::ArrayList<_Polygon2DContour*> loops;

private:
    _Polygon2DContour* currentLoop;
    BinaryTreeNode<_Polygon2DContour*>* headNode;

    static bool isPointOnSegment(const Vertex2D& p, const Vertex2D& a, const Vertex2D& b);
    static bool isContourInsideContour(const java::ArrayList<Vertex2D>& contour, const java::ArrayList<Vertex2D>& container);

public:
    Polygon2D();
    virtual ~Polygon2D();

    void addVertex(double x, double y, double r, double g, double b);
    void addVertex(double x, double y);
    void pushVertex(double x, double y);

    void nextLoop();
    void eraseLastLoop();
    void invert();
    void classifyContourHoles();

    static signed char isPointInsideContour(const Vertex2D& point, const java::ArrayList<Vertex2D>& contour);

    virtual double* getMinMax();

    Ray* doIntersectionFirstHit(const Ray& inOut_ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double intT, RayHit* outData);

    BinaryTreeNode<_Polygon2DContour*>* getHeadNode();
    void setHeadNode(BinaryTreeNode<_Polygon2DContour*>* headNode);
};

#endif
