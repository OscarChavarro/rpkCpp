#ifndef __VERTEX__
#define __VERTEX__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Triangle;

/**
A vertex is a basic data pack for tipically used operations in computer
graphics, as polygon mesh representations and basic visualization algorithms
and shaders.

This class is meant as a modeling facility for polygon meshes, and a common
structure for operation signatures.

Note that this is NOT a class, but merely a data structure, and as such,
all its attributes are public.  This structure is not supposed to evolve or
change in time radically.

This class is supposed to be used as basic element for building polygons in
three dimensional space. Note that is being currently used in polygon meshes
(surfaces as such TriangleMesh) and on structured solid polygons, as such
BinarySpacePartitioningTreeSolid.
*/
class Vertex
{
public:
    Vector3Dd position;
    Vector3Dd normal;
    Vector3Dd binormal;
    Vector3Dd tangent;

    /// Texture coordinates
    double u;
    double v;

    java::ArrayList<Triangle>* incidentTriangles;

    Vertex();
    Vertex(double x, double y, double z);
    explicit Vertex(const Vector3Dd& position);
    Vertex(const Vector3Dd& position, const Vector3Dd& normal);
    Vertex(const Vector3Dd& position, const Vector3Dd& normal, double u, double v);
    Vertex(const Vector3Dd& position, const Vector3Dd& normal, const Vector3Dd& binormal, const Vector3Dd& tangent);

    Vector3Dd getPosition() const;
    Vector3Dd getNormal() const;
    Vector3Dd getBinormal() const;
    Vector3Dd getTangent() const;
    double getU() const;
    double getV() const;
    java::ArrayList<Triangle>* getIncidentTriangles() const;

    void setPosition(const Vector3Dd& position);
    void setNormal(const Vector3Dd& normal);
    void setBinormal(const Vector3Dd& binormal);
    void setTangent(const Vector3Dd& tangent);
    void setU(double u);
    void setV(double v);
    void setIncidentTriangles(java::ArrayList<Triangle>* incidentTriangles);

    Triangle getIncidentTriangleAt(int index) const;

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable representation of current vertex
    */
    java::String toString() const;
};

#endif
