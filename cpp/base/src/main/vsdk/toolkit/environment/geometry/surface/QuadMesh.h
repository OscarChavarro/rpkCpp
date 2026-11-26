#ifndef __QUAD_MESH__
#define __QUAD_MESH__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class Vertex;
class Ray;
class RayHit;
class TriangleMeshGroup;

class QuadMesh : public Surface {
private:
    java::String name;

    java::ArrayList<double> vertexPositions;
    java::ArrayList<double> vertexNormals;
    java::ArrayList<double> vertexBinormals;
    java::ArrayList<double> vertexTangents;
    java::ArrayList<double> vertexColors;
    java::ArrayList<double> vertexUvs;

    java::ArrayList< java::ArrayList<int> > incidentQuadsPerVertexArray;
    java::ArrayList<int> quadIndices;

    double* calculateMinMaxPositions();

public:
    QuadMesh();

    java::String getName() const;
    void setName(const java::String& name);

    void getVertexAt(int i, Vertex& vertex) const;

    void initVertexPositionsArray(int n);
    void initVertexColorsArray();
    void initVertexNormalsArray();

    void setVertexes(const java::ArrayList<Vertex>& vertexes);
    void initQuadArrays(int n);

    void setVertexAt(int i, const Vertex& vertex);
    void setQuadAt(int i, int p0, int p1, int p2, int p3);

    int getNumVertices() const;
    int getNumQuads() const;

    java::ArrayList<double>& getVertexPositions();
    java::ArrayList<double>& getVertexNormals();
    java::ArrayList<double>& getVertexBinormals();
    java::ArrayList<double>& getVertexTangents();
    java::ArrayList<double>& getVertexColors();
    java::ArrayList<double>& getVertexUvs();
    java::ArrayList<int>& getQuadIndices();

    void calculateNormals();

    virtual double* getMinMax();

    TriangleMeshGroup* exportToTriangleMeshGroup();

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    java::String toString() const;
};

#endif
