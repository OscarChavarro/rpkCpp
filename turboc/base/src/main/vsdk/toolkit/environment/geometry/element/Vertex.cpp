#include <stddef.h>
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
Vertex::Vertex()
    : position(0, 0, 0), normal(1, 0, 0), binormal(0, 1, 0), tangent(0, 0, 1),
      u(0.0), v(0.0), incidentTriangles(NULL) {}

Vertex::Vertex(double x, double y, double z)
    : position(x, y, z), normal(1, 0, 0), binormal(0, 1, 0), tangent(0, 0, 1),
      u(0.0), v(0.0), incidentTriangles(NULL) {}

Vertex::Vertex(const Vector3Dd& position)
    : position(position), normal(1, 0, 0), binormal(0, 1, 0), tangent(0, 0, 1),
      u(0.0), v(0.0), incidentTriangles(NULL) {}

Vertex::Vertex(const Vector3Dd& position, const Vector3Dd& normal)
    : position(position), normal(normal.normalized()), binormal(0, 1, 0), tangent(0, 0, 1),
      u(0.0), v(0.0), incidentTriangles(NULL) {}

Vertex::Vertex(const Vector3Dd& position, const Vector3Dd& normal, double inU, double inV)
    : position(position), normal(normal.normalized()), binormal(0, 1, 0), tangent(0, 0, 1),
      u(inU), v(inV), incidentTriangles(NULL) {}

Vertex::Vertex(const Vector3Dd& position, const Vector3Dd& normal, const Vector3Dd& binormal, const Vector3Dd& tangent)
    : position(position), normal(normal), binormal(binormal), tangent(tangent),
      u(0.0), v(0.0), incidentTriangles(NULL) {}

Vertex::Vertex(const Vertex& vertex)
    : position(vertex.position), normal(vertex.normal), binormal(vertex.binormal), tangent(vertex.tangent),
      u(vertex.u), v(vertex.v), incidentTriangles(vertex.incidentTriangles) {}

Vector3Dd Vertex::getPosition() const { return position; }
Vector3Dd Vertex::getNormal() const { return normal; }
Vector3Dd Vertex::getBinormal() const { return binormal; }
Vector3Dd Vertex::getTangent() const { return tangent; }
double Vertex::getU() const { return u; }
double Vertex::getV() const { return v; }
ArrayList<Triangle>* Vertex::getIncidentTriangles() const { return incidentTriangles; }

void Vertex::setPosition(const Vector3Dd& inPosition) { position = Vector3Dd(inPosition); }
void Vertex::setNormal(const Vector3Dd& inNormal) { normal = Vector3Dd(inNormal).normalized(); }
void Vertex::setBinormal(const Vector3Dd& inBinormal) { binormal = inBinormal; }
void Vertex::setTangent(const Vector3Dd& inTangent) { tangent = inTangent; }
void Vertex::setU(double inU) { u = inU; }
void Vertex::setV(double inV) { v = inV; }
void Vertex::setIncidentTriangles(ArrayList<Triangle>* inIncidentTriangles) { incidentTriangles = inIncidentTriangles; }

Triangle Vertex::getIncidentTriangleAt(int index) const
{
    return incidentTriangles->get(index);
}

String Vertex::toString() const
{
    return "v <" + VSDK::formatDouble(position.x()) + ", " +
        VSDK::formatDouble(position.y()) + ", " +
        VSDK::formatDouble(position.z()) + "> n <" +
        VSDK::formatDouble(normal.x()) + ", " +
        VSDK::formatDouble(normal.y()) + ", " +
        VSDK::formatDouble(normal.z()) + "> UV<" +
        VSDK::formatDouble(u) + ", " + VSDK::formatDouble(v) + ">";
}
