#include <cstdio>

#include "java/lang/Math.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
#include "vsdk/toolkit/environment/geometry/surface/QuadMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
QuadMesh::QuadMesh() : name("default")
{
}

java::String QuadMesh::getName() const
{
    return name;
}

void QuadMesh::setName(const java::String& inName)
{
    name = inName;
}

void QuadMesh::getVertexAt(int i, Vertex& vertex) const
{
    vertex.position = Vector3Dd(vertexPositions.get(3*i), vertexPositions.get(3*i+1), vertexPositions.get(3*i+2));
    if ((int)vertexNormals.size() >= (i+1)*3) {
        vertex.normal = Vector3Dd(vertexNormals.get(3*i), vertexNormals.get(3*i+1), vertexNormals.get(3*i+2));
    }
    if ((int)vertexBinormals.size() >= (i+1)*3) {
        vertex.binormal = Vector3Dd(vertexBinormals.get(3*i), vertexBinormals.get(3*i+1), vertexBinormals.get(3*i+2));
    }
    if ((int)vertexTangents.size() >= (i+1)*3) {
        vertex.tangent = Vector3Dd(vertexTangents.get(3*i), vertexTangents.get(3*i+1), vertexTangents.get(3*i+2));
    }
    if ((int)vertexUvs.size() >= (i+1)*2) {
        vertex.u = vertexUvs.get(2*i);
        vertex.v = vertexUvs.get(2*i+1);
    }
}

void QuadMesh::initVertexPositionsArray(int n)
{
    vertexPositions.clear(); vertexPositions.reserve((long int)n*3);
    for (long int i = 0; i < (long int)n*3; i++) vertexPositions.add(0.0);
    incidentQuadsPerVertexArray.clear(); incidentQuadsPerVertexArray.reserve((long int)n);
    for (long int i = 0; i < (long int)n; i++) incidentQuadsPerVertexArray.add(java::ArrayList<int>());
}

void QuadMesh::initVertexColorsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexColors.clear(); vertexColors.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexColors.add(0.0);
}

void QuadMesh::initVertexNormalsArray()
{
    long int nn = (long int)getNumVertices()*3;
    vertexNormals.clear(); vertexNormals.reserve(nn);
    for (long int i = 0; i < nn; i++) vertexNormals.add(0.0);
}

void QuadMesh::setVertexes(const java::ArrayList<Vertex>& vertexes)
{
    initVertexPositionsArray((int)vertexes.size());
    initVertexNormalsArray();
    for (int i = 0; i < (int)vertexes.size(); i++) {
        Vertex v = vertexes.get(i);
        vertexPositions[3*i] = v.position.x();
        vertexPositions[3*i+1] = v.position.y();
        vertexPositions[3*i+2] = v.position.z();
        vertexNormals[3*i] = v.normal.x();
        vertexNormals[3*i+1] = v.normal.y();
        vertexNormals[3*i+2] = v.normal.z();
    }
}

void QuadMesh::initQuadArrays(int n)
{
    quadIndices.clear(); quadIndices.reserve((long int)n*4);
    for (long int i = 0; i < (long int)n*4; i++) quadIndices.add(0);
}

void QuadMesh::setVertexAt(int i, const Vertex& vertex)
{
    vertexPositions[3*i] = vertex.position.x();
    vertexPositions[3*i+1] = vertex.position.y();
    vertexPositions[3*i+2] = vertex.position.z();
    if ((int)vertexNormals.size() >= (i+1)*3) {
        vertexNormals[3*i] = vertex.normal.x();
        vertexNormals[3*i+1] = vertex.normal.y();
        vertexNormals[3*i+2] = vertex.normal.z();
    }
    if ((int)vertexBinormals.size() >= (i+1)*3) {
        vertexBinormals[3*i] = vertex.binormal.x();
        vertexBinormals[3*i+1] = vertex.binormal.y();
        vertexBinormals[3*i+2] = vertex.binormal.z();
    }
    if ((int)vertexTangents.size() >= (i+1)*3) {
        vertexTangents[3*i] = vertex.tangent.x();
        vertexTangents[3*i+1] = vertex.tangent.y();
        vertexTangents[3*i+2] = vertex.tangent.z();
    }
    if ((int)vertexUvs.size() >= (i+1)*2) {
        vertexUvs[2*i] = vertex.u;
        vertexUvs[2*i+1] = vertex.v;
    }
}

void QuadMesh::setQuadAt(int i, int p0, int p1, int p2, int p3)
{
    quadIndices[4*i] = p0;
    quadIndices[4*i+1] = p1;
    quadIndices[4*i+2] = p2;
    quadIndices[4*i+3] = p3;
}

int QuadMesh::getNumVertices() const
{
    return (int)vertexPositions.size()/3;
}

int QuadMesh::getNumQuads() const
{
    return (int)quadIndices.size()/4;
}

java::ArrayList<double>& QuadMesh::getVertexPositions() { return vertexPositions; }
java::ArrayList<double>& QuadMesh::getVertexNormals() { return vertexNormals; }
java::ArrayList<double>& QuadMesh::getVertexBinormals() { return vertexBinormals; }
java::ArrayList<double>& QuadMesh::getVertexTangents() { return vertexTangents; }
java::ArrayList<double>& QuadMesh::getVertexColors() { return vertexColors; }
java::ArrayList<double>& QuadMesh::getVertexUvs() { return vertexUvs; }
java::ArrayList<int>& QuadMesh::getQuadIndices() { return quadIndices; }

void QuadMesh::calculateNormals()
{
    initVertexNormalsArray();
    for (int i = 0; i < getNumQuads(); i++) {
        int i0 = quadIndices[4*i];
        int i1 = quadIndices[4*i+1];
        int i2 = quadIndices[4*i+2];
        int i3 = quadIndices[4*i+3];
        if (i0 < 0 || i1 < 0 || i2 < 0 || i3 < 0 ||
            i0 >= getNumVertices() || i1 >= getNumVertices() ||
            i2 >= getNumVertices() || i3 >= getNumVertices()) {
            continue;
        }

        Vector3Dd v0(vertexPositions[3*i0], vertexPositions[3*i0+1], vertexPositions[3*i0+2]);
        Vector3Dd v1(vertexPositions[3*i1], vertexPositions[3*i1+1], vertexPositions[3*i1+2]);
        Vector3Dd v2(vertexPositions[3*i2], vertexPositions[3*i2+1], vertexPositions[3*i2+2]);
        Vector3Dd v3(vertexPositions[3*i3], vertexPositions[3*i3+1], vertexPositions[3*i3+2]);

        Vector3Dd n1 = v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized();
        Vector3Dd n2 = v2.subtract(v0).crossProduct(v3.subtract(v0)).normalized();
        Vector3Dd n = n1.add(n2).normalized();

        int ids[4] = {i0, i1, i2, i3};
        for (int k = 0; k < 4; k++) {
            int idx = ids[k];
            vertexNormals[3*idx] += n.x();
            vertexNormals[3*idx+1] += n.y();
            vertexNormals[3*idx+2] += n.z();
        }
    }

    for (int i = 0; i < getNumVertices(); i++) {
        Vector3Dd n(vertexNormals[3*i], vertexNormals[3*i+1], vertexNormals[3*i+2]);
        n = n.normalized();
        vertexNormals[3*i] = n.x();
        vertexNormals[3*i+1] = n.y();
        vertexNormals[3*i+2] = n.z();
    }
}

double* QuadMesh::calculateMinMaxPositions()
{
    double* minMax = new double[6];
    if (getNumVertices() == 0) {
        minMax[0] = minMax[1] = minMax[2] = 0.0;
        minMax[3] = minMax[4] = minMax[5] = 0.0;
        return minMax;
    }

    double minX = 1e308, minY = 1e308, minZ = 1e308;
    double maxX = -1e308, maxY = -1e308, maxZ = -1e308;

    for (int i = 0; i < getNumVertices(); i++) {
        double x = vertexPositions[3*i];
        double y = vertexPositions[3*i+1];
        double z = vertexPositions[3*i+2];
        minX = java::Math::min(minX, x);
        minY = java::Math::min(minY, y);
        minZ = java::Math::min(minZ, z);
        maxX = java::Math::max(maxX, x);
        maxY = java::Math::max(maxY, y);
        maxZ = java::Math::max(maxZ, z);
    }

    minMax[0] = minX; minMax[1] = minY; minMax[2] = minZ;
    minMax[3] = maxX; minMax[4] = maxY; minMax[5] = maxZ;
    return minMax;
}

double* QuadMesh::getMinMax()
{
    return calculateMinMaxPositions();
}

TriangleMeshGroup* QuadMesh::exportToTriangleMeshGroup()
{
    TriangleMesh mesh;
    mesh.setName(name);
    mesh.getVertexPositions() = vertexPositions;
    mesh.getVertexNormals() = vertexNormals;
    mesh.getVertexBinormals() = vertexBinormals;
    mesh.getVertexTangents() = vertexTangents;
    mesh.getVertexColors() = vertexColors;
    mesh.getVertexUvs() = vertexUvs;

    mesh.initTriangleArrays(getNumQuads()*2);
    java::ArrayList<int>& tri = mesh.getTriangleIndexes();

    int t = 0;
    for (int i = 0; i < getNumQuads(); i++) {
        int p0 = quadIndices[4*i];
        int p1 = quadIndices[4*i+1];
        int p2 = quadIndices[4*i+2];
        int p3 = quadIndices[4*i+3];
        tri[3*t] = p0; tri[3*t+1] = p1; tri[3*t+2] = p2; t++;
        tri[3*t] = p0; tri[3*t+1] = p2; tri[3*t+2] = p3; t++;
    }
    mesh.calculateNormals();

    TriangleMeshGroup* g = new TriangleMeshGroup();
    g->addMesh(mesh);
    return g;
}

Ray* QuadMesh::doIntersectionFirstHit(const Ray& inOut_Ray)
{
    TriangleMeshGroup* g = exportToTriangleMeshGroup();
    Ray* out = g->doIntersectionFirstHit(inOut_Ray);
    delete g;
    return out;
}

bool QuadMesh::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    TriangleMeshGroup* g = exportToTriangleMeshGroup();
    bool out = g->doIntersectionFirstHit(inRay, outHit);
    delete g;
    return out;
}

void QuadMesh::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    TriangleMeshGroup* g = exportToTriangleMeshGroup();
    g->doExtraInformation(inRay, inT, outData);
    delete g;
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

java::String QuadMesh::toString() const
{
    java::String msg;
    QuadMesh* self = const_cast<QuadMesh*>(this);
    double* mm = self->getMinMax();
    msg += "- QuadMesh ------------------------------------------------------------\n";
    msg += "  - Number of quads:";
    msg += intToStr(getNumQuads());
    msg += "\n";
    msg += "  - Number of vertexes:";
    msg += intToStr(getNumVertices());
    msg += "\n";
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
    msg += "---------------------------------------------------------------------------\n";
    delete [] mm;
    return msg;
}
