#ifndef __QUAD_MESH__
#define __QUAD_MESH__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
class Ray;
class RayHit;
class TriangleMeshGroup;

class QuadMesh : public Surface {
private:
    String name;

    ArrayList<double> vertexPositions;
    ArrayList<double> vertexNormals;
    ArrayList<double> vertexBinormals;
    ArrayList<double> vertexTangents;
    ArrayList<double> vertexColors;
    ArrayList<double> vertexUvs;

    ArrayList< ArrayList<int> > incidentQuadsPerVertexArray;
    ArrayList<int> quadIndices;

    double* calculateMinMaxPositions();

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    QuadMesh();

    String getName() const;
    void setName(const String& name);

    void getVertexAt(int i, Vertex& vertex) const;

    void initVertexPositionsArray(int n);
    void initVertexColorsArray();
    void initVertexNormalsArray();

    void setVertexes(const ArrayList<Vertex>& vertexes);
    void initQuadArrays(int n);

    void setVertexAt(int i, const Vertex& vertex);
    void setQuadAt(int i, int p0, int p1, int p2, int p3);

    int getNumVertices() const;
    int getNumQuads() const;

    ArrayList<double>& getVertexPositions();
    ArrayList<double>& getVertexNormals();
    ArrayList<double>& getVertexBinormals();
    ArrayList<double>& getVertexTangents();
    ArrayList<double>& getVertexColors();
    ArrayList<double>& getVertexUvs();
    ArrayList<int>& getQuadIndices();

    void calculateNormals();

    virtual double* getMinMax();

    TriangleMeshGroup* exportToTriangleMeshGroup();

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    String toString() const;
};

#endif
