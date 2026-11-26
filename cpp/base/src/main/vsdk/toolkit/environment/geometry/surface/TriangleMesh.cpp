#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
/*
This class represents a "basic" triangle mesh. Its model is based in a set
of vertexes and triangles (the edges are not store explicitly, and there
can not be an edge not forming part of a triangle).

This basic triangle mesh model can be associated with one and only one
material (this could change in future), but can have multiple textures,
and each texture can be mapped to a different set of triangles.

As every model class or `Entity` in VSDK, this class only can represent
(store in memory) the mesh model. It doesn't provide persistence or rendering
functionality, as this could be found at `io` and `render` packages.
Nevertheles, this class will be highly coupled with both of those, so
making any change here will impact highly that code.

This class does not ensure nor impose data integrity, and this will be the
sole responsability of the cooperating utilities and applications.
*/

TriangleMesh::TriangleMesh() : name("default"), ownsMaterials(false) {}

TriangleMesh::TriangleMesh(const TriangleMesh& o)
    : name(o.name), vertexPositions(o.vertexPositions), vertexNormals(o.vertexNormals),
      vertexBinormals(o.vertexBinormals), vertexTangents(o.vertexTangents),
      vertexColors(o.vertexColors), vertexSelections(o.vertexSelections),
      vertexUvs(o.vertexUvs), incidentTrianglesPerVertexArray(o.incidentTrianglesPerVertexArray),
      triangleIndexes(o.triangleIndexes), triangleNormals(o.triangleNormals),
      materials(o.materials), textureRanges(o.textureRanges), materialRanges(o.materialRanges),
      textures(o.textures), intersectionTriangleIndex(o.intersectionTriangleIndex),
      ownsMaterials(false), accelerationStructure(o.accelerationStructure)
{
}

TriangleMesh::~TriangleMesh()
{
    if (ownsMaterials) {
        for (long int i = 0; i < materials.size(); i++) {
            if (materials[i] != 0) {
                delete materials[i];
            }
        }
    }
}

TriangleMesh* TriangleMesh::clone() const { return new TriangleMesh(*this); }

Geometry* TriangleMesh::copyGeometry() const
{
    TriangleMesh* copy = new TriangleMesh(*this);
    copy->materials.clear();
    copy->textures.clear();
    copy->materialRanges.clear();
    copy->textureRanges.clear();
    return copy;
}

java::String TriangleMesh::getName() const { return name; }
void TriangleMesh::setName(const java::String& n) { name = n; }

void TriangleMesh::getVertexAt(int i, Vertex& vertex) const
{
    vertex.position = Vector3Dd(vertexPositions.get(i*3), vertexPositions.get(i*3+1), vertexPositions.get(i*3+2));
    if ((int)vertexNormals.size() >= (i+1)*3) vertex.normal = Vector3Dd(vertexNormals.get(i*3), vertexNormals.get(i*3+1), vertexNormals.get(i*3+2));
    if ((int)vertexBinormals.size() >= (i+1)*3) vertex.binormal = Vector3Dd(vertexBinormals.get(i*3), vertexBinormals.get(i*3+1), vertexBinormals.get(i*3+2));
    if ((int)vertexTangents.size() >= (i+1)*3) vertex.tangent = Vector3Dd(vertexTangents.get(i*3), vertexTangents.get(i*3+1), vertexTangents.get(i*3+2));
    if ((int)vertexUvs.size() >= (i+1)*2) { vertex.u = vertexUvs.get(i*2); vertex.v = vertexUvs.get(i*2+1); }
}

void TriangleMesh::initVertexPositionsArray(int n)
{
    accelerationStructure.reset(0);
    vertexPositions.clear(); vertexPositions.reserve((long int)n*3);
    for (long int i = 0; i < (long int)n*3; i++) vertexPositions.add(0.0);
    vertexSelections.clear(); vertexSelections.reserve((long int)n);
    for (long int i = 0; i < (long int)n; i++) vertexSelections.add(false);
}
void TriangleMesh::initVertexNormalsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexNormals.clear(); vertexNormals.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexNormals.add(0.0);
}
void TriangleMesh::initVertexBinormalsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexBinormals.clear(); vertexBinormals.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexBinormals.add(0.0);
}
void TriangleMesh::initVertexTangentsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexTangents.clear(); vertexTangents.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexTangents.add(0.0);
}
void TriangleMesh::initVertexColorsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexColors.clear(); vertexColors.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexColors.add(0.0);
}
void TriangleMesh::initVertexUvsArray()
{
    long int nn = (long int)getNumVertices()*2;
    vertexUvs.clear(); vertexUvs.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexUvs.add(0.0);
}
void TriangleMesh::initIncidentTrianglesPerVertexArray()
{
    int nv = getNumVertices();
    incidentTrianglesPerVertexArray.clear(); incidentTrianglesPerVertexArray.reserve((long int)nv);
    for (int i = 0; i < nv; i++) incidentTrianglesPerVertexArray.add(java::ArrayList<int>());
}

void TriangleMesh::detachColors() { vertexColors.clear(); }
void TriangleMesh::detachNormals() { vertexNormals.clear(); }
void TriangleMesh::detachUvs() { vertexUvs.clear(); }

/*
This method provides a clear structured form of defining the mesh vertexes,
but it is inefficient. Its use is discouraged for applications manipulating
big meshes.
@param vertexes
@param withNormals
@param withBinormals
@param withTangents
@param withUvs
*/
void TriangleMesh::setVertexes(const java::ArrayList<Vertex>& vertexes, bool withNormals, bool withBinormals, bool withTangents, bool withUvs)
{
    initVertexPositionsArray((int)vertexes.size());
    if (withNormals) initVertexNormalsArray();
    if (withBinormals) initVertexBinormalsArray();
    if (withTangents) initVertexTangentsArray();
    if (withUvs) initVertexUvsArray();
    for (int i = 0; i < (int)vertexes.size(); i++) setVertexAt(i, vertexes.get(i));
}

void TriangleMesh::initTriangleArrays(int n)
{
    accelerationStructure.reset(0);
    triangleIndexes.clear(); triangleIndexes.reserve((long int)n*3);
    for (long int i = 0; i < (long int)n*3; i++) triangleIndexes.add(0);
    triangleNormals.clear(); triangleNormals.reserve((long int)n*3);
    for (long int i = 0; i < (long int)n*3; i++) triangleNormals.add(0.0);
}

/*
This method provides a clear structured form of defining the mesh triangles,
but it is inefficient. Its use is discouraged for applications manipulating
big meshes.
@param triangles
*/
void TriangleMesh::setTriangles(const java::ArrayList<Triangle>& triangles)
{
    initTriangleArrays((int)triangles.size());
    for (int i = 0; i < (int)triangles.size(); i++) setTriangleAt(i, triangles.get(i));
}

void TriangleMesh::setTextures(const java::ArrayList<Image*>& t) { textures = t; }
void TriangleMesh::setMaterials(const java::ArrayList<SimpleMaterial*>& m) { materials = m; }
void TriangleMesh::setOwnsMaterials(bool owns) { ownsMaterials = owns; }

/*
Given a vertex structure and an `i` position, this method copies
the information from the structure in to the i-th vertex arrays position.
PRE: 0 <= i < vertexPositions.length/3
@param i
@param vertex
*/
void TriangleMesh::setVertexAt(int i, const Vertex& v)
{
    accelerationStructure.reset(0);
    vertexPositions[i*3] = v.position.x();
    vertexPositions[i*3+1] = v.position.y();
    vertexPositions[i*3+2] = v.position.z();
    if ((int)vertexNormals.size() >= (i+1)*3) {
        vertexNormals[i*3] = v.normal.x(); vertexNormals[i*3+1] = v.normal.y(); vertexNormals[i*3+2] = v.normal.z();
    }
    if ((int)vertexBinormals.size() >= (i+1)*3) {
        vertexBinormals[i*3] = v.binormal.x(); vertexBinormals[i*3+1] = v.binormal.y(); vertexBinormals[i*3+2] = v.binormal.z();
    }
    if ((int)vertexTangents.size() >= (i+1)*3) {
        vertexTangents[i*3] = v.tangent.x(); vertexTangents[i*3+1] = v.tangent.y(); vertexTangents[i*3+2] = v.tangent.z();
    }
    if ((int)vertexUvs.size() >= (i+1)*2) {
        vertexUvs[i*2] = v.u; vertexUvs[i*2+1] = v.v;
    }
}

/*
Given a triangle structure and an `i` position, this method copies
the information from the structure in to the i-th triangle arrays position.
PRE: 0 <= i < vertexPositions.length/3
@param i
@param triangle
*/
void TriangleMesh::setTriangleAt(int i, const Triangle& t)
{
    accelerationStructure.reset(0);
    triangleIndexes[i*3] = t.getPoint0();
    triangleIndexes[i*3+1] = t.getPoint1();
    triangleIndexes[i*3+2] = t.getPoint2();
    if ((int)triangleNormals.size() >= (i+1)*3) {
        triangleNormals[i*3] = t.getNormal().x();
        triangleNormals[i*3+1] = t.getNormal().y();
        triangleNormals[i*3+2] = t.getNormal().z();
    }
}

int TriangleMesh::getNumVertices() const { return (int)vertexPositions.size()/3; }
int TriangleMesh::getNumTriangles() const { return (int)triangleIndexes.size()/3; }

java::ArrayList<bool>& TriangleMesh::getVertexSelections()
{
    if ((int)vertexSelections.size() != getNumVertices()) {
        int nv = getNumVertices();
        vertexSelections.clear(); vertexSelections.reserve((long int)nv);
        for (int i = 0; i < nv; i++) vertexSelections.add(false);
    }
    return vertexSelections;
}
java::ArrayList<double>& TriangleMesh::getVertexPositions() { return vertexPositions; }
java::ArrayList<double>& TriangleMesh::getVertexNormals() { return vertexNormals; }
java::ArrayList<double>& TriangleMesh::getVertexBinormals() { return vertexBinormals; }
java::ArrayList<double>& TriangleMesh::getVertexTangents() { return vertexTangents; }
java::ArrayList<double>& TriangleMesh::getVertexColors() { return vertexColors; }
java::ArrayList<double>& TriangleMesh::getVertexUvs() { return vertexUvs; }
java::ArrayList<int>& TriangleMesh::getTriangleIndexes() { return triangleIndexes; }
java::ArrayList<double>& TriangleMesh::getTriangleNormals() { return triangleNormals; }

java::ArrayList<SimpleMaterial*>& TriangleMesh::getMaterials() { return materials; }
java::ArrayList<Image*>& TriangleMesh::getTextures() { return textures; }
Image* TriangleMesh::getTextureAt(int index) const { return (index>=0 && index<(int)textures.size()) ? textures.get(index) : nullptr; }
void TriangleMesh::setTextureAt(int index, Image* image) { if (index>=0 && index<(int)textures.size()) textures[index] = image; }

java::ArrayList< java::ArrayList<int> >& TriangleMesh::getTextureRanges() { return textureRanges; }
/*
Note this always returns an array with two (2) integers: the first one
is an index to `triangles` array, the second one is an index to the
`textures` array.
@param spanRange
@return an integer array for textures ranges indexes
*/
java::ArrayList<int> TriangleMesh::getTextureRangeAt(int spanRange) const { return textureRanges.get(spanRange); }
void TriangleMesh::setTextureRanges(const java::ArrayList< java::ArrayList<int> >& r) { textureRanges = r; }

java::ArrayList< java::ArrayList<int> >& TriangleMesh::getMaterialRanges() { return materialRanges; }
/*
Note this always returns an array with two (2) integers: the first one
is an index to `triangles` array, the second one is an index to the
`materials` array.
@param spanRange
@return a integer array with material ranges indexes
*/
java::ArrayList<int> TriangleMesh::getMaterialRangeAt(int spanRange) const { return materialRanges.get(spanRange); }
void TriangleMesh::setMaterialRanges(const java::ArrayList< java::ArrayList<int> >& r) { materialRanges = r; }

void TriangleMesh::calculateNormals()
{
    initVertexNormalsArray();
    {
        long int nn = (long int)getNumTriangles()*3;
        triangleNormals.clear(); triangleNormals.reserve(nn);
        for (long int i = 0; i < nn; i++) triangleNormals.add(0.0);
    }

    for (int i = 0; i < getNumTriangles(); i++) {
        int i0 = triangleIndexes[i*3], i1 = triangleIndexes[i*3+1], i2 = triangleIndexes[i*3+2];
        Vector3Dd v0(vertexPositions[i0*3], vertexPositions[i0*3+1], vertexPositions[i0*3+2]);
        Vector3Dd v1(vertexPositions[i1*3], vertexPositions[i1*3+1], vertexPositions[i1*3+2]);
        Vector3Dd v2(vertexPositions[i2*3], vertexPositions[i2*3+1], vertexPositions[i2*3+2]);
        Vector3Dd n = v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized();

        triangleNormals[i*3] = n.x(); triangleNormals[i*3+1] = n.y(); triangleNormals[i*3+2] = n.z();

        vertexNormals[i0*3] += n.x(); vertexNormals[i0*3+1] += n.y(); vertexNormals[i0*3+2] += n.z();
        vertexNormals[i1*3] += n.x(); vertexNormals[i1*3+1] += n.y(); vertexNormals[i1*3+2] += n.z();
        vertexNormals[i2*3] += n.x(); vertexNormals[i2*3+1] += n.y(); vertexNormals[i2*3+2] += n.z();
    }

    for (int i = 0; i < getNumVertices(); i++) {
        Vector3Dd n(vertexNormals[i*3], vertexNormals[i*3+1], vertexNormals[i*3+2]);
        n = n.normalized();
        vertexNormals[i*3] = n.x(); vertexNormals[i*3+1] = n.y(); vertexNormals[i*3+2] = n.z();
    }
}

void TriangleMesh::reorientateNormals()
{
    for (long int i = 0; i < vertexNormals.size(); i++) vertexNormals[i] = -vertexNormals[i];
    for (int i = 0; i < getNumTriangles(); i++) {
        int tmp = triangleIndexes[i*3+1];
        triangleIndexes[i*3+1] = triangleIndexes[i*3+2];
        triangleIndexes[i*3+2] = tmp;
    }
}

double* TriangleMesh::calculateMinMaxPositions()
{
    double* m = new double[6];
    double minX=1e308,minY=1e308,minZ=1e308,maxX=-1e308,maxY=-1e308,maxZ=-1e308;
    for (int i = 0; i < getNumVertices(); i++) {
        double x=vertexPositions[i*3], y=vertexPositions[i*3+1], z=vertexPositions[i*3+2];
        minX=java::Math::min(minX,x); minY=java::Math::min(minY,y); minZ=java::Math::min(minZ,z);
        maxX=java::Math::max(maxX,x); maxY=java::Math::max(maxY,y); maxZ=java::Math::max(maxZ,z);
    }
    m[0]=minX; m[1]=minY; m[2]=minZ; m[3]=maxX; m[4]=maxY; m[5]=maxZ;
    return m;
}

void TriangleMesh::getBounds(GeometryBounds& result) const
{
    if (triangleIndexes.size() % 3 != 0 || vertexPositions.size() % 3 != 0) {
        result.setIndeterminate(); return;
    }
    if (getNumTriangles() == 0) { result.setEmpty(); return; }
    Vector3Dd minimum, maximum;
    bool first = true;
    for (long int i = 0; i < triangleIndexes.size(); ++i) {
        const int index = triangleIndexes[i];
        if (index < 0 || index >= getNumVertices()) { result.setIndeterminate(); return; }
        const double x = vertexPositions[index*3], y = vertexPositions[index*3+1];
        const double z = vertexPositions[index*3+2];
        if (!java::Double::isFinite(x) || !java::Double::isFinite(y) || !java::Double::isFinite(z)) {
            result.setIndeterminate(); return;
        }
        if (first) { minimum = maximum = Vector3Dd(x,y,z); first = false; }
        else {
            minimum = Vector3Dd(java::Math::min(minimum.x(),x), java::Math::min(minimum.y(),y), java::Math::min(minimum.z(),z));
            maximum = Vector3Dd(java::Math::max(maximum.x(),x), java::Math::max(maximum.y(),y), java::Math::max(maximum.z(),z));
        }
    }
    if (first) result.setEmpty();
    else result.setFinite(minimum, maximum);
}

bool TriangleMesh::getTrianglePoints(int index, Vector3Dd& p0, Vector3Dd& p1, Vector3Dd& p2) const
{
    if (index < 0 || index >= getNumTriangles() || vertexPositions.size() % 3 != 0 ||
        triangleIndexes.size() % 3 != 0) return false;
    const int a = triangleIndexes[index*3], b = triangleIndexes[index*3+1], c = triangleIndexes[index*3+2];
    if (a < 0 || b < 0 || c < 0 || a >= getNumVertices() || b >= getNumVertices() || c >= getNumVertices())
        return false;
    p0 = Vector3Dd(vertexPositions[a*3], vertexPositions[a*3+1], vertexPositions[a*3+2]);
    p1 = Vector3Dd(vertexPositions[b*3], vertexPositions[b*3+1], vertexPositions[b*3+2]);
    p2 = Vector3Dd(vertexPositions[c*3], vertexPositions[c*3+1], vertexPositions[c*3+2]);
    return true;
}

void TriangleMesh::fillTypedDetail(const Ray& ray, double t, int face, double u, double v,
    const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2,
    bool normalReversed, RayHit* hit) const
{
    if (!hit || !hit->needsAnySurfaceData()) return;
    hit->beginDetailEvaluation();
    if (hit->needsPoint()) {
        hit->point = ray.getOrigin().add(ray.getDirection().multiply(t));
        hit->markDetailAvailable(RayHit::DETAIL_POINT);
    }
    if (hit->needsNormal()) {
        const Vector3Dd normal = p1.subtract(p0).crossProduct(p2.subtract(p0));
        const double squared = normal.dotProduct(normal);
        if (java::Double::isFinite(squared) && squared > 0) {
            hit->normal = squared < VSDK::EPSILON ? normal.multiply(1.0/java::Math::sqrt(squared)) : normal.normalized();
            if (normalReversed) hit->normal = hit->normal.multiply(-1);
            hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }
    if (hit->needsTextureCoordinates() && vertexUvs.size() >= (long)getNumVertices()*2) {
        const int a = triangleIndexes[face*3], b = triangleIndexes[face*3+1], c = triangleIndexes[face*3+2];
        const double w = 1.0-u-v;
        const double texU = w*vertexUvs[a*2] + u*vertexUvs[b*2] + v*vertexUvs[c*2];
        const double texV = w*vertexUvs[a*2+1] + u*vertexUvs[b*2+1] + v*vertexUvs[c*2+1];
        if (java::Double::isFinite(texU) && java::Double::isFinite(texV)) {
            hit->u = texU; hit->v = texV; hit->markDetailAvailable(RayHit::DETAIL_UV);
        }
    }
}

RayFirstHitStatus TriangleMesh::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const
{
    if (hit) hit->clear();
    if (boundary) *boundary = RayBoundary();
    if (triangleIndexes.size() % 3 != 0 || vertexPositions.size() % 3 != 0)
        return RayFirstHitStatus::Indeterminate;
    double t = 0, u = 0, v = 0;
    if (getNumTriangles() == 0) {
        const Vector3Dd zero;
        return Triangle::queryIntersection(ray, zero, zero, zero, query, t, u, v);
    }
    int best = -1;
    double bestT = 0, bestU = 0, bestV = 0;
    Vector3Dd bestP0, bestP1, bestP2;
    for (int i = 0; i < getNumTriangles(); ++i) {
        Vector3Dd p0, p1, p2;
        if (!getTrianglePoints(i, p0, p1, p2)) return RayFirstHitStatus::Indeterminate;
        const RayFirstHitStatus status = Triangle::queryIntersection(ray, p0, p1, p2, query, t, u, v);
        if (status != RayFirstHitStatus::Hit && status != RayFirstHitStatus::Miss) return status;
        if (status == RayFirstHitStatus::Hit && (best < 0 || t < bestT)) {
            best = i; bestT = t; bestU = u; bestV = v; bestP0 = p0; bestP1 = p1; bestP2 = p2;
        }
    }
    if (best < 0) return RayFirstHitStatus::Miss;
    const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(bestT));
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return RayFirstHitStatus::Indeterminate;
    if (boundary) *boundary = RayBoundary(bestT, this, false, best);
    if (hit) {
        hit->setHitDistance(bestT);
        if (hit->shouldStoreRay()) hit->setRay(ray.withT(bestT));
        fillTypedDetail(ray, bestT, best, bestU, bestV, bestP0, bestP1, bestP2, false, hit);
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus TriangleMesh::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
{
    if (hit) hit->clear();
    if (!hit || boundary.geometry != this || boundary.surfaceIndex < 0 ||
        boundary.surfaceIndex >= getNumTriangles() || !java::Double::isFinite(boundary.t))
        return RayFirstHitStatus::InvalidInput;
    if (boundary.getInstanceCount() != 0) return RayFirstHitStatus::Unsupported;
    const int face = (int)boundary.surfaceIndex;
    Vector3Dd p0, p1, p2;
    if (!getTrianglePoints(face, p0, p1, p2)) return RayFirstHitStatus::Indeterminate;
    double t = 0, u = 0, v = 0;
    const RayFirstHitStatus status = Triangle::queryIntersection(ray, p0, p1, p2,
        RayCrossingQuery(boundary.t, boundary.t), t, u, v);
    if (status != RayFirstHitStatus::Hit) return status;
    const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return RayFirstHitStatus::Indeterminate;
    hit->setHitDistance(t);
    if (hit->shouldStoreRay()) hit->setRay(ray.withT(t));
    fillTypedDetail(ray, t, face, u, v, p0, p1, p2, boundary.normalReversed, hit);
    return RayFirstHitStatus::Hit;
}

RayCrossingSupport TriangleMesh::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    if (triangleIndexes.size() % 3 != 0 || vertexPositions.size() % 3 != 0)
        return RayCrossingSupport::Indeterminate;
    java::ArrayList<double> depths;
    java::ArrayList<int> faces;
    double t = 0, u = 0, v = 0;
    if (getNumTriangles() == 0) {
        const Vector3Dd zero;
        const RayFirstHitStatus status = Triangle::queryIntersection(ray, zero, zero, zero, query, t, u, v);
        if (status == RayFirstHitStatus::Unsupported) return RayCrossingSupport::Unsupported;
        if (status == RayFirstHitStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
        if (status == RayFirstHitStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
        return RayCrossingSupport::Supported;
    }
    for (int i = 0; i < getNumTriangles(); ++i) {
        Vector3Dd p0, p1, p2;
        if (!getTrianglePoints(i, p0, p1, p2)) return RayCrossingSupport::Indeterminate;
        const RayFirstHitStatus status = Triangle::queryIntersection(ray, p0, p1, p2, query, t, u, v);
        if (status == RayFirstHitStatus::Unsupported) return RayCrossingSupport::Unsupported;
        if (status == RayFirstHitStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
        if (status == RayFirstHitStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
        if (status == RayFirstHitStatus::Hit) {
            const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
            if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
                !java::Double::isFinite(point.z())) return RayCrossingSupport::Indeterminate;
            long int at = 0;
            while (at < depths.size() && depths[at] <= t) ++at;
            if (at == depths.size()) { depths.add(t); faces.add(i); }
            else { depths.add(at, t); faces.add(at, i); }
        }
    }
    for (long int i = 0; i < depths.size(); ++i) {
        RayCrossing event(depths[i], this, RayCrossing::Kind::Unclassified);
        event.surfaceIndex = faces[i];
        sink.crossing(event);
    }
    return RayCrossingSupport::Supported;
}

int TriangleMesh::doIntersectionInformation() const { return (int)intersectionTriangleIndex.get(); }

void TriangleMesh::prepareAccelerationStructure(TriangleMeshAccelerationMethod method)
{
    // Done here, single threaded, instead of lazily from the tracing threads.
    if ((int)triangleNormals.size() < getNumTriangles()*3) calculateNormals();
    if (method == TriangleMeshAccelerationMethod::BRUTE_FORCE) {
        if (accelerationStructure.get() != 0) accelerationStructure.reset(0);
        return;
    }
    const TriangleAccelerationStructure* current = accelerationStructure.get();
    if (current != 0 && current->getMethod() == method &&
        current->matches(getNumVertices(), getNumTriangles())) return;
    TriangleAccelerationStructure* structure = TriangleAccelerationStructure::create(method);
    if (structure != 0) structure->build(vertexPositions, triangleIndexes);
    accelerationStructure.reset(structure);
}

void TriangleMesh::releaseAccelerationStructure() { accelerationStructure.reset(0); }

const TriangleAccelerationStructure* TriangleMesh::getAccelerationStructure() const
{
    return accelerationStructure.get();
}

bool TriangleMesh::getPreparedMinMax(double* outMinMax) const
{
    const TriangleAccelerationStructure* structure = accelerationStructure.get();
    if (structure == 0 || !structure->matches(getNumVertices(), getNumTriangles())) return false;
    structure->getVertexBounds(outMinMax);
    return true;
}

Ray* TriangleMesh::doIntersectionFirstHit(const Ray& inOut_Ray)
{
    RayHit hit;
    if (doIntersectionFirstHit(inOut_Ray, &hit) && hit.getRay() != 0) return new Ray(*hit.getRay());
    return 0;
}

void TriangleMesh::interpolateTriangleData(int tri, double u, double v, RayHit* outHit, const Vector3Dd& rayDirection)
{
    if (outHit == 0) return;
    int i0 = triangleIndexes[tri*3], i1 = triangleIndexes[tri*3+1], i2 = triangleIndexes[tri*3+2];
    double w = 1.0 - u - v;

    if (outHit->needsNormal()) {
        if ((int)vertexNormals.size() >= getNumVertices()*3 && (int)vertexUvs.size() >= getNumVertices()*2) {
            Vector3Dd n(
                w*vertexNormals[i0*3] + u*vertexNormals[i1*3] + v*vertexNormals[i2*3],
                w*vertexNormals[i0*3+1] + u*vertexNormals[i1*3+1] + v*vertexNormals[i2*3+1],
                w*vertexNormals[i0*3+2] + u*vertexNormals[i1*3+2] + v*vertexNormals[i2*3+2]);
            outHit->normal = n.normalized();
        }
        else {
            Vector3Dd n(triangleNormals[tri*3], triangleNormals[tri*3+1], triangleNormals[tri*3+2]);
            outHit->normal = n.normalized();
        }
        if (outHit->normal.dotProduct(rayDirection) >= 0) outHit->normal = outHit->normal.multiply(-1);
    }

    if (outHit->needsTextureCoordinates() && (int)vertexUvs.size() >= getNumVertices()*2) {
        outHit->u = w*vertexUvs[i0*2] + u*vertexUvs[i1*2] + v*vertexUvs[i2*2];
        outHit->v = w*vertexUvs[i0*2+1] + u*vertexUvs[i1*2+1] + v*vertexUvs[i2*2+1];
    }
}

void TriangleMesh::fillMaterialAndTexture(int triangleIndex, RayHit* outHit)
{
    if (outHit == 0) return;
    if (materials.size() != 0) outHit->material = materials[0];

    if (materialRanges.size() != 0) {
        for (long int i = 0; i + 1 < materialRanges.size(); i++) {
            if (materialRanges[i].size() >= 1 && materialRanges[i+1].size() >= 2 &&
                triangleIndex >= materialRanges[i][0] && triangleIndex < materialRanges[i+1][0]) {
                int idx = materialRanges[i+1][1];
                if (idx >= 0 && idx < (int)materials.size()) outHit->material = materials[idx];
                break;
            }
        }
    }

    outHit->texture = 0;
    if (textureRanges.size() != 0) {
        for (long int i = 0; i + 1 < textureRanges.size(); i++) {
            if (textureRanges[i].size() >= 1 && textureRanges[i+1].size() >= 2 &&
                triangleIndex >= textureRanges[i][0] && triangleIndex < textureRanges[i+1][0]) {
                int idx = textureRanges[i+1][1] - 1;
                if (idx >= 0 && idx < (int)textures.size()) outHit->texture = textures[idx];
                break;
            }
        }
    }
}

bool TriangleMesh::doIntersectionInternal(const Ray& inRay, RayHit* outHit, int* outTriangleIndex)
{
    if (getNumTriangles() == 0) {
        intersectionTriangleIndex.set(-1);
        if (outTriangleIndex != 0) *outTriangleIndex = -1;
        return false;
    }
    if ((int)triangleNormals.size() < getNumTriangles()*3) calculateNormals();

    double bestT = 1e308;
    int bestTri = -1;
    double bestU = 0, bestV = 0;
    bool found;

    const TriangleAccelerationStructure* structure = accelerationStructure.get();
    if (structure != 0 && structure->matches(getNumVertices(), getNumTriangles())) {
        found = structure->firstHit(inRay, vertexPositions, triangleIndexes,
            bestT, bestU, bestV, bestTri);
    }
    else {
        found = TriangleAccelerationStructure::firstHitBruteForce(inRay,
            vertexPositions, triangleIndexes, bestT, bestU, bestV, bestTri);
    }

    if (!found) {
        intersectionTriangleIndex.set(-1);
        if (outTriangleIndex != 0) *outTriangleIndex = -1;
        return false;
    }
    intersectionTriangleIndex.set(bestTri);
    if (outTriangleIndex != 0) *outTriangleIndex = bestTri;

    if (outHit != 0) {
        outHit->setRay(inRay.withT(bestT));
        outHit->point = inRay.getOrigin().add(inRay.getDirection().multiply(bestT));
        outHit->normal = Vector3Dd(triangleNormals[bestTri*3], triangleNormals[bestTri*3+1], triangleNormals[bestTri*3+2]).normalized();
        outHit->tangent = Vector3Dd();
        outHit->u = 0;
        outHit->v = 0;
        interpolateTriangleData(bestTri, bestU, bestV, outHit, inRay.getDirection());
        fillMaterialAndTexture(bestTri, outHit);
    }
    return true;
}

bool TriangleMesh::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    return doIntersectionInternal(inRay, outHit, 0);
}

bool TriangleMesh::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit, int* outTriangleIndex)
{
    return doIntersectionInternal(inRay, outHit, outTriangleIndex);
}

void TriangleMesh::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if (outData == 0) return;
    RayHit hit;
    if (doIntersectionFirstHit(inRay.withT(inT), &hit)) outData->clone(hit);
}

int TriangleMesh::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    for (int i = 0; i < getNumTriangles(); i++) {
        Vector3Dd p0(vertexPositions[3*triangleIndexes[3*i+0]+0], vertexPositions[3*triangleIndexes[3*i+0]+1], vertexPositions[3*triangleIndexes[3*i+0]+2]);
        Vector3Dd p1(vertexPositions[3*triangleIndexes[3*i+1]+0], vertexPositions[3*triangleIndexes[3*i+1]+1], vertexPositions[3*triangleIndexes[3*i+1]+2]);
        Vector3Dd p2(vertexPositions[3*triangleIndexes[3*i+2]+0], vertexPositions[3*triangleIndexes[3*i+2]+1], vertexPositions[3*triangleIndexes[3*i+2]+2]);
        int status = Triangle::containmentTest(p0, p1, p2, p, distanceTolerance);
        if (status != OUTSIDE) return LIMIT;
    }
    return OUTSIDE;
}

double* TriangleMesh::getMinMax() { return calculateMinMaxPositions(); }

TriangleMeshGroup* TriangleMesh::exportToTriangleMeshGroup()
{
    TriangleMeshGroup* g = new TriangleMeshGroup();
    g->addMesh(*this);
    return g;
}

void TriangleMesh::compact()
{
    accelerationStructure.reset(0);
    vertexNormals.clear();

    int n = getNumVertices();
    java::ArrayList<bool> count;
    count.reserve((long int)n);
    for (long int i = 0; i < (long int)n; i++) count.add(false);
    for (long int i = 0; i < triangleIndexes.size(); i++) {
        int a = triangleIndexes[i];
        if (a >= 0 && a < n) count[a] = true;
    }

    int j = 0;
    java::ArrayList<int> map;
    map.reserve((long int)n);
    for (long int i = 0; i < (long int)n; i++) map.add(-1);
    for (int i = 0; i < n; i++) if (count[i]) map[i] = j++;

    java::ArrayList<double> oldPos = vertexPositions;
    java::ArrayList<double> oldNor = vertexNormals;

    initVertexPositionsArray(j);
    if (oldNor.size() != 0) initVertexNormalsArray();

    j = 0;
    for (int i = 0; i < n; i++) {
        if (count[i]) {
            vertexPositions[3*j+0] = oldPos[3*i+0];
            vertexPositions[3*j+1] = oldPos[3*i+1];
            vertexPositions[3*j+2] = oldPos[3*i+2];
            if (oldNor.size() != 0) {
                vertexNormals[3*j+0] = oldNor[3*i+0];
                vertexNormals[3*j+1] = oldNor[3*i+1];
                vertexNormals[3*j+2] = oldNor[3*i+2];
            }
            j++;
        }
    }

    java::ArrayList<int> oldTri = triangleIndexes;
    j = 0;
    for (int i = 0; i < (int)oldTri.size()/3; i++) {
        int a = oldTri[3*i+0], b = oldTri[3*i+1], c = oldTri[3*i+2];
        if (!(a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n)) j++;
    }

    initTriangleArrays(j);
    j = 0;
    for (int i = 0; i < (int)oldTri.size()/3; i++) {
        int a = oldTri[3*i+0], b = oldTri[3*i+1], c = oldTri[3*i+2];
        if (a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n) {}
        else {
            triangleIndexes[3*j+0] = map[a];
            triangleIndexes[3*j+1] = map[b];
            triangleIndexes[3*j+2] = map[c];
            j++;
        }
    }
}

void TriangleMesh::removeSelectedVertices()
{
    accelerationStructure.reset(0);
    if (vertexSelections.size() == 0) return;

    vertexNormals.clear();
    vertexBinormals.clear();
    vertexTangents.clear();

    int n = getNumVertices();
    for (int i = 0; i < (int)triangleIndexes.size()/3; i++) {
        int a = triangleIndexes[3*i+0];
        int b = triangleIndexes[3*i+1];
        int c = triangleIndexes[3*i+2];
        if (a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n ||
            vertexSelections[a] || vertexSelections[b] || vertexSelections[c]) {
            triangleIndexes[3*i+0] = -1;
            triangleIndexes[3*i+1] = -1;
            triangleIndexes[3*i+2] = -1;
        }
    }

    compact();
    vertexSelections.clear();
    calculateNormals();
}

void TriangleMesh::appendVertices(const java::ArrayList<double>& ev)
{
    for (long int i = 0; i < ev.size(); i++) vertexPositions.add(ev.get(i));
}

void TriangleMesh::appendTriangles(const java::ArrayList<int>& et)
{
    for (long int i = 0; i < et.size(); i++) triangleIndexes.add(et.get(i));
}

void TriangleMesh::simpleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                                     java::ArrayList<int>& extraTriangles, int nv, int i,
                                     const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3)
{
    Vector3Dd a = p2.subtract(p1).normalized();
    Vector3Dd b = p3.subtract(p1).normalized();

    Vector3Dd ma, mb;
    bool hasMa = false, hasMb = false;

    Ray* hitA = p.doIntersectionWithNegative(Ray(p1, a));
    if (hitA != 0) {
        RayHit gia;
        if (p.doIntersectionFirstHit(*hitA, &gia)) { ma = gia.point; hasMa = true; }
        delete hitA;
    }
    Ray* hitB = p.doIntersectionWithNegative(Ray(p1, b));
    if (hitB != 0) {
        RayHit gib;
        if (p.doIntersectionFirstHit(*hitB, &gib)) { mb = gib.point; hasMb = true; }
        delete hitB;
    }

    extraTriangles.add(i);
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    if (!hasMa || !hasMb) return;

    extraVertices.add(ma.x()); extraVertices.add(ma.y()); extraVertices.add(ma.z());
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    extraVertices.add(mb.x()); extraVertices.add(mb.y()); extraVertices.add(mb.z());
}

void TriangleMesh::halfTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                                   java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                                   const Vector3Dd&, const Vector3Dd& p2, const Vector3Dd& p3)
{
    Vector3Dd a = p2.subtract(p3).normalized();

    Vector3Dd ma;
    bool hasMa = false;

    Ray* hitA = p.doIntersectionWithNegative(Ray(p2, a));
    if (hitA != 0) {
        RayHit gia;
        if (p.doIntersectionFirstHit(*hitA, &gia)) { ma = gia.point; hasMa = true; }
        delete hitA;
    }

    extraTriangles.add(i);
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    if (!hasMa) return;

    extraVertices.add(ma.x()); extraVertices.add(ma.y()); extraVertices.add(ma.z());
    extraTriangles.add(j);
}

void TriangleMesh::doubleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                                     java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                                     const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3)
{
    Vector3Dd a = p1.subtract(p3).normalized();
    Vector3Dd b = p2.subtract(p3).normalized();

    Vector3Dd ma, mb;
    bool hasMa = false, hasMb = false;

    Ray* hitA = p.doIntersectionWithNegative(Ray(p3, a));
    if (hitA != 0) {
        RayHit gia;
        if (p.doIntersectionFirstHit(*hitA, &gia)) { ma = gia.point; hasMa = true; }
        delete hitA;
    }
    Ray* hitB = p.doIntersectionWithNegative(Ray(p3, b));
    if (hitB != 0) {
        RayHit gib;
        if (p.doIntersectionFirstHit(*hitB, &gib)) { mb = gib.point; hasMb = true; }
        delete hitB;
    }

    extraTriangles.add(i);
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    if (!hasMa || !hasMb) return;

    extraVertices.add(ma.x()); extraVertices.add(ma.y()); extraVertices.add(ma.z());
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    extraTriangles.add(i);
    extraTriangles.add((int)extraVertices.size()/3 + nv);
    extraTriangles.add(j);
    extraVertices.add(mb.x()); extraVertices.add(mb.y()); extraVertices.add(mb.z());
}

void TriangleMesh::slice(InfinitePlane& p)
{
    accelerationStructure.reset(0);
    java::ArrayList<int> extraTriangles;
    java::ArrayList<double> extraVertices;

    int nv = getNumVertices();

    for (int i = 0; i < (int)triangleIndexes.size()/3; i++) {
        int ia = triangleIndexes[3*i+0];
        int ib = triangleIndexes[3*i+1];
        int ic = triangleIndexes[3*i+2];
        if (ia < 0 || ib < 0 || ic < 0 || ia >= getNumVertices() || ib >= getNumVertices() || ic >= getNumVertices()) continue;

        Vector3Dd p1(vertexPositions[3*ia+0], vertexPositions[3*ia+1], vertexPositions[3*ia+2]);
        Vector3Dd p2(vertexPositions[3*ib+0], vertexPositions[3*ib+1], vertexPositions[3*ib+2]);
        Vector3Dd p3(vertexPositions[3*ic+0], vertexPositions[3*ic+1], vertexPositions[3*ic+2]);

        int t1 = p.doContainmentTestHalfSpace(p1, VSDK::EPSILON);
        int t2 = p.doContainmentTestHalfSpace(p2, VSDK::EPSILON);
        int t3 = p.doContainmentTestHalfSpace(p3, VSDK::EPSILON);

        if ((t1 == OUTSIDE && t2 == OUTSIDE && t3 == OUTSIDE) ||
            (t1 == LIMIT && t2 == OUTSIDE && t3 == OUTSIDE) ||
            (t1 == OUTSIDE && t2 == LIMIT && t3 == OUTSIDE) ||
            (t1 == OUTSIDE && t2 == OUTSIDE && t3 == LIMIT) ||
            (t1 == LIMIT && t2 == LIMIT && t3 == OUTSIDE) ||
            (t1 == LIMIT && t2 == OUTSIDE && t3 == LIMIT) ||
            (t1 == OUTSIDE && t2 == LIMIT && t3 == LIMIT)) {
            triangleIndexes[3*i+0] = triangleIndexes[3*i+1] = triangleIndexes[3*i+2] = -1;
        }
        else if (t1 == LIMIT && t2 == LIMIT && t3 == LIMIT) {
            Vector3Dd n = p2.subtract(p1).crossProduct(p3.subtract(p1));
            if (n.dotProduct(p.getNormal()) > 0) triangleIndexes[3*i+0] = triangleIndexes[3*i+1] = triangleIndexes[3*i+2] = -1;
        }
        else if ((t1 == LIMIT && t2 == INSIDE && t3 == INSIDE) ||
                 (t1 == INSIDE && t2 == LIMIT && t3 == INSIDE) ||
                 (t1 == INSIDE && t2 == INSIDE && t3 == LIMIT) ||
                 (t1 == LIMIT && t2 == LIMIT && t3 == INSIDE) ||
                 (t1 == LIMIT && t2 == INSIDE && t3 == LIMIT) ||
                 (t1 == INSIDE && t2 == LIMIT && t3 == LIMIT) ||
                 (t1 == INSIDE && t2 == INSIDE && t3 == INSIDE)) {
        }
        else {
            if (t1 == INSIDE && t2 == OUTSIDE && t3 == OUTSIDE) simpleTriangleCut(p, extraVertices, extraTriangles, nv, ia, p1, p2, p3);
            else if (t2 == INSIDE && t1 == OUTSIDE && t3 == OUTSIDE) simpleTriangleCut(p, extraVertices, extraTriangles, nv, ia, p2, p1, p3);
            else if (t3 == INSIDE && t1 == OUTSIDE && t2 == OUTSIDE) simpleTriangleCut(p, extraVertices, extraTriangles, nv, ia, p3, p1, p2);
            else if (t1 == INSIDE && t2 == INSIDE && t3 == OUTSIDE) doubleTriangleCut(p, extraVertices, extraTriangles, nv, ia, ib, p1, p2, p3);
            else if (t1 == INSIDE && t3 == INSIDE && t2 == OUTSIDE) doubleTriangleCut(p, extraVertices, extraTriangles, nv, ia, ic, p1, p3, p2);
            else if (t2 == INSIDE && t3 == INSIDE && t1 == OUTSIDE) doubleTriangleCut(p, extraVertices, extraTriangles, nv, ib, ic, p2, p3, p1);
            else if (t1 == INSIDE && t2 == LIMIT && t3 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ib, p2, p1, p3);
            else if (t2 == INSIDE && t1 == LIMIT && t3 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ia, p1, p2, p3);
            else if (t2 == INSIDE && t3 == LIMIT && t1 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ic, p3, p2, p1);
            else if (t3 == INSIDE && t2 == LIMIT && t1 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ib, p2, p3, p1);
            else if (t3 == INSIDE && t1 == LIMIT && t2 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ia, p1, p3, p2);
            else if (t1 == INSIDE && t3 == LIMIT && t2 == OUTSIDE) halfTriangleCut(p, extraVertices, extraTriangles, nv, ia, ic, p3, p1, p2);
            else Logger::reportMessage("TriangleMesh", Logger::WARNING, "slice", "Unhandled slice case");
        }
    }

    appendVertices(extraVertices);
    appendTriangles(extraTriangles);
    compact();
    calculateNormals();
}

static java::String doubleToStr(double val) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", val);
    return java::String(buf);
}

static java::String intToStr(int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return java::String(buf);
}

java::String TriangleMesh::toString() const
{
    java::String msg;
    msg += "- TriangleMesh ------------------------------------------------------------\n";
    msg += "  - Number of triangles:";
    msg += intToStr(getNumTriangles());
    msg += "\n";
    msg += "  - Number of vertexes:";
    msg += intToStr(getNumVertices());
    msg += "\n";

    TriangleMesh* self = const_cast<TriangleMesh*>(this);
    double* mm = self->getMinMax();
    msg += "  - MINMAX: (";
    msg += doubleToStr(mm[0]);
    msg += ", ";
    msg += doubleToStr(mm[1]);
    msg += ", ";
    msg += doubleToStr(mm[2]);
    msg += ") - (";
    msg += doubleToStr(mm[3]);
    msg += ", ";
    msg += doubleToStr(mm[4]);
    msg += ", ";
    msg += doubleToStr(mm[5]);
    msg += ")\n";
    delete [] mm;

    if (materials.size() == 0) msg += "  - No materials available!\n";
    else {
        msg += "  - ";
        msg += intToStr((int)materials.size());
        msg += " materials\n";
    }

    if (materialRanges.size() == 0) msg += "  - No material ranges association table available!\n";
    else {
        msg += "  - ";
        msg += intToStr((int)materialRanges.size());
        msg += " material spans:\n";
        for (long int i = 0; i < materialRanges.size(); i++) {
            java::ArrayList<int> mr = materialRanges.get(i);
            if (mr.size() >= 2) {
                msg += "    . ";
                msg += intToStr(mr[0]);
                msg += " -> ";
                msg += intToStr(mr[1]);
                msg += "\n";
            }
        }
    }

    if (textures.size() == 0) msg += "  - No textures available!\n";
    else {
        msg += "  - ";
        msg += intToStr((int)textures.size());
        msg += " textures\n";
    }

    if (textureRanges.size() == 0) msg += "  - No texture ranges association table available!\n";
    else {
        msg += "  - ";
        msg += intToStr((int)textureRanges.size());
        msg += " texture spans:\n";
        for (long int i = 0; i < textureRanges.size(); i++) {
            java::ArrayList<int> tr = textureRanges.get(i);
            if (tr.size() >= 2) {
                msg += "    . ";
                msg += intToStr(tr[0]);
                msg += " -> ";
                msg += intToStr(tr[1]);
                msg += "\n";
            }
        }
    }

    msg += "---------------------------------------------------------------------------\n";
    return msg;
}
