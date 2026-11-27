#ifndef __POLYGON_2_D__
#define __POLYGON_2_D__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
// C++ port note: Vertex2D is complete here, as gcc 2.7.2 instantiates
// ArrayList<Vertex2D> where the parameters of isPointInsideContour name it
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
class _Polygon2DContour;
class Ray;
class RayHit;
template <class T> class BinaryTreeNode;

class Polygon2D : public Surface
{
public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    ArrayList<_Polygon2DContour*> loops;

private:
    _Polygon2DContour* currentLoop;
    BinaryTreeNode<_Polygon2DContour*>* headNode;

    static BOOLEAN isPointOnSegment(const Vertex2D& p, const Vertex2D& a, const Vertex2D& b);
    static BOOLEAN isContourInsideContour(const ArrayList<Vertex2D>& contour, const ArrayList<Vertex2D>& container);

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

    static signed char isPointInsideContour(const Vertex2D& point, const ArrayList<Vertex2D>& contour);

    virtual double* getMinMax();

    Ray* doIntersectionFirstHit(const Ray& inOut_ray);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double intT, RayHit* outData);

    BinaryTreeNode<_Polygon2DContour*>* getHeadNode();
    void setHeadNode(BinaryTreeNode<_Polygon2DContour*>* headNode);
};

#endif
