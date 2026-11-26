#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/dataStructures/BinaryTreeNode.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
#include <cmath>

Polygon2D::Polygon2D() : currentLoop(nullptr), headNode(nullptr) { nextLoop(); }

Polygon2D::~Polygon2D()
{
    for (long int i = 0; i < loops.size(); i++) delete loops[i];
}

void Polygon2D::addVertex(double x, double y, double r, double g, double b) { currentLoop->addVertex(x,y,r,g,b); }
void Polygon2D::addVertex(double x, double y) { currentLoop->addVertex(x,y); }
void Polygon2D::pushVertex(double x, double y) { currentLoop->pushVertex(x,y); }

void Polygon2D::nextLoop()
{
    currentLoop = new _Polygon2DContour();
    loops.add(currentLoop);
}

void Polygon2D::eraseLastLoop()
{
    if (loops.size() <= 1) return;
    _Polygon2DContour* last = loops[loops.size() - 1];
    for (long int i = 0; i < loops.size(); ) {
        if (loops[i]->getExteriorContour() == last) {
            delete loops[i];
            loops.remove(i);
        }
        else i++;
    }
    if (loops.size() > 1) {
        delete loops[loops.size() - 1];
        loops.remove(loops.size() - 1);
        currentLoop = loops[loops.size() - 1];
    }
    classifyContourHoles();
}

void Polygon2D::invert()
{
    for (long int i = 0; i < loops.size(); i++) {
        java::ArrayList<Vertex2D>& verts = loops[i]->vertices;
        long int _n = verts.size();
        for (long int _i = 0; _i < _n / 2; _i++) {
            auto _tmp = verts[_i]; verts[_i] = verts[_n - 1 - _i]; verts[_n - 1 - _i] = _tmp;
        }
    }
}

double* Polygon2D::getMinMax()
{
    double* minMax = new double[6];
    double minX = 1e308, minY = 1e308;
    double maxX = -1e308, maxY = -1e308;

    for (long int i = 0; i < loops.size(); i++) {
        for (long int j = 0; j + 1 < loops[i]->vertices.size(); j++) {
            const Vertex2D& v = loops[i]->vertices[j];
            if (v.x > maxX) maxX = v.x;
            if (v.x < minX) minX = v.x;
            if (v.y > maxY) maxY = v.y;
            if (v.y < minY) minY = v.y;
        }
    }

    minMax[0]=minX; minMax[1]=minY; minMax[2]=0;
    minMax[3]=maxX; minMax[4]=maxY; minMax[5]=0;
    return minMax;
}

static bool pointInLoop(const _Polygon2DContour* loop, double x, double y)
{
    bool inside = false;
    java::ArrayList<Vertex2D>& v = const_cast<_Polygon2DContour*>(loop)->vertices;
    long int n = v.size();
    if (n < 3) return false;
    for (long int i = 0, j = n - 1; i < n; j = i++) {
        bool inter = ((v[i].y > y) != (v[j].y > y)) &&
                     (x < (v[j].x - v[i].x) * (y - v[i].y) / ((v[j].y - v[i].y) + 1e-30) + v[i].x);
        if (inter) inside = !inside;
    }
    return inside;
}

Ray* Polygon2D::doIntersectionFirstHit(const Ray& inOut_ray)
{
    RayHit hit;
    if (doIntersectionFirstHit(inOut_ray, &hit) && hit.getRay() != nullptr) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool Polygon2D::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    if (std::abs(inRay.getDirection().z()) < VSDK::EPSILON) return false;
    double t = -inRay.getOrigin().z() / inRay.getDirection().z();
    if (t < 0) return false;

    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(t));
    bool inside = false;
    for (long int i = 0; i < loops.size(); i++) {
        if (pointInLoop(loops[i], p.x(), p.y())) {
            if (loops[i]->getExteriorContour() == nullptr) inside = true;
            else inside = false;
        }
    }
    if (!inside) return false;

    if (outHit != nullptr) {
        if (outHit->shouldStoreRay() || outHit->needsAnySurfaceData()) outHit->setRay(inRay.withT(t));
        else outHit->setHitDistance(t);
        if (outHit->needsPoint()) outHit->point = p;
        if (outHit->needsNormal()) outHit->normal = Vector3Dd(0,0,1);
    }
    return true;
}

void Polygon2D::doExtraInformation(const Ray& inRay, double intT, RayHit* outData)
{
    if (outData == nullptr) return;
    outData->point = inRay.getOrigin().add(inRay.getDirection().multiply(intT));
    outData->normal = Vector3Dd(0,0,1);
}

BinaryTreeNode<_Polygon2DContour*>* Polygon2D::getHeadNode() { return headNode; }
void Polygon2D::setHeadNode(BinaryTreeNode<_Polygon2DContour*>* node) { headNode = node; }

bool Polygon2D::isPointOnSegment(const Vertex2D& p, const Vertex2D& a, const Vertex2D& b)
{
    double cross = (p.y-a.y)*(b.x-a.x) - (p.x-a.x)*(b.y-a.y);
    if (std::fabs(cross) > 1e-12) return false;
    double dot = (p.x-a.x)*(b.x-a.x) + (p.y-a.y)*(b.y-a.y);
    if (dot < 0) return false;
    double len2 = (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
    return dot <= len2;
}

signed char Polygon2D::isPointInsideContour(const Vertex2D& point, const java::ArrayList<Vertex2D>& polygon)
{
    bool inside = false;
    long int n = polygon.size();
    if (n < 3) return -1;
    for (long int i = 0, j = n - 1; i < n; j = i++) {
        Vertex2D vi = polygon.get(i);
        Vertex2D vj = polygon.get(j);
        if (isPointOnSegment(point, vj, vi)) return 0;
        bool intersect = ((vi.y > point.y) != (vj.y > point.y)) &&
                         (point.x < (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + 1e-30) + vi.x);
        if (intersect) inside = !inside;
    }
    return inside ? 1 : -1;
}

bool Polygon2D::isContourInsideContour(const java::ArrayList<Vertex2D>& contour, const java::ArrayList<Vertex2D>& mainPolygon)
{
    for (long int i = 0; i < contour.size(); ++i) {
        signed char r = isPointInsideContour(contour.get(i), mainPolygon);
        if (r == 1) return true;
        if (r == -1) return false;
    }
    return false;
}

void Polygon2D::classifyContourHoles()
{
    for (long int i = 0; i < loops.size(); ++i) loops[i]->setExteriorContour(0);
    BinaryTreeNode<_Polygon2DContour*>* root = new BinaryTreeNode<_Polygon2DContour*>(0);
    setHeadNode(root);
    for (long int i = 0; i < loops.size(); ++i) {
        _Polygon2DContour* a = loops[i];
        for (long int j = 0; j < loops.size(); ++j) {
            if (i == j) continue;
            _Polygon2DContour* b = loops[j];
            if (isContourInsideContour(a->vertices, b->vertices)) {
                a->setExteriorContour(b);
                break;
            }
        }
    }
}
