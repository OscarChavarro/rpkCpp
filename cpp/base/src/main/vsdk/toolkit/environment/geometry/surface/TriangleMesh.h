#ifndef __TRIANGLE_MESH__
#define __TRIANGLE_MESH__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshIntersectionIndex.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class Vertex;
class Triangle;
class SimpleMaterial;
class Image;
class Ray;
class RayHit;
class TriangleMeshGroup;
class InfinitePlane;

class TriangleMesh : public Surface {
private:
    java::String name;

    java::ArrayList<double> vertexPositions;
    java::ArrayList<double> vertexNormals;
    java::ArrayList<double> vertexBinormals;
    java::ArrayList<double> vertexTangents;
    java::ArrayList<double> vertexColors;
    java::ArrayList<bool> vertexSelections;
    java::ArrayList<double> vertexUvs;

    java::ArrayList< java::ArrayList<int> > incidentTrianglesPerVertexArray;

    java::ArrayList<int> triangleIndexes;
    java::ArrayList<double> triangleNormals;

    java::ArrayList<SimpleMaterial*> materials;
    java::ArrayList<Image*> textures;

    java::ArrayList< java::ArrayList<int> > textureRanges;
    java::ArrayList< java::ArrayList<int> > materialRanges;

    TriangleMeshIntersectionIndex intersectionTriangleIndex;
    bool ownsMaterials;
    // Optional ray acceleration structure, see prepareAccelerationStructure
    TriangleAccelerationStructureSlot accelerationStructure;

    void interpolateTriangleData(int triangleIndex, double u, double v, RayHit* outHit, const Vector3Dd& rayDirection);
    void fillMaterialAndTexture(int triangleIndex, RayHit* outHit);
    bool doIntersectionInternal(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);
    bool getTrianglePoints(int index, Vector3Dd& p0, Vector3Dd& p1, Vector3Dd& p2) const;
    void fillTypedDetail(const Ray& ray, double t, int face, double u, double v,
        const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2,
        bool normalReversed, RayHit* hit) const;

    double* calculateMinMaxPositions();

    void appendVertices(const java::ArrayList<double>& ev);
    void appendTriangles(const java::ArrayList<int>& et);

    void simpleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                           java::ArrayList<int>& extraTriangles, int nv, int i,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void halfTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                         java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                         const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void doubleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                           java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);

public:
    TriangleMesh();
    TriangleMesh(const TriangleMesh& other);
    ~TriangleMesh();

    TriangleMesh* clone() const;
    // Mathematical copy; scene-owned materials and textures are not copied.
    Geometry* copyGeometry() const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;

    java::String getName() const;
    void setName(const java::String& name);

    void getVertexAt(int i, Vertex& vertex) const;

    void initVertexPositionsArray(int n);
    void initVertexNormalsArray();
    void initVertexBinormalsArray();
    void initVertexTangentsArray();
    void initVertexColorsArray();
    void initVertexUvsArray();
    void initIncidentTrianglesPerVertexArray();

    void detachColors();
    void detachNormals();
    void detachUvs();

    void setVertexes(const java::ArrayList<Vertex>& vertexes, bool withNormals, bool withBinormals, bool withTangents, bool withUvs);

    void initTriangleArrays(int n);
    void setTriangles(const java::ArrayList<Triangle>& triangles);

    void setTextures(const java::ArrayList<Image*>& textures);
    void setMaterials(const java::ArrayList<SimpleMaterial*>& materials);
    void setOwnsMaterials(bool owns);

    void setVertexAt(int i, const Vertex& vertex);
    void setTriangleAt(int i, const Triangle& triangle);

    int getNumVertices() const;
    int getNumTriangles() const;

    java::ArrayList<bool>& getVertexSelections();

    java::ArrayList<double>& getVertexPositions();
    const java::ArrayList<double>& getVertexPositionsView() const { return vertexPositions; }
    java::ArrayList<double>& getVertexNormals();
    java::ArrayList<double>& getVertexBinormals();
    java::ArrayList<double>& getVertexTangents();
    java::ArrayList<double>& getVertexColors();
    java::ArrayList<double>& getVertexUvs();
    java::ArrayList<int>& getTriangleIndexes();
    java::ArrayList<double>& getTriangleNormals();
    const java::ArrayList<int>& getTriangleIndexesView() const { return triangleIndexes; }
    const java::ArrayList<double>& getTriangleNormalsView() const { return triangleNormals; }

    java::ArrayList<SimpleMaterial*>& getMaterials();
    java::ArrayList<Image*>& getTextures();
    Image* getTextureAt(int index) const;
    void setTextureAt(int index, Image* image);

    java::ArrayList< java::ArrayList<int> >& getTextureRanges();
    java::ArrayList<int> getTextureRangeAt(int spanRange) const;
    void setTextureRanges(const java::ArrayList< java::ArrayList<int> >& ranges);

    java::ArrayList< java::ArrayList<int> >& getMaterialRanges();
    java::ArrayList<int> getMaterialRangeAt(int spanRange) const;
    void setMaterialRanges(const java::ArrayList< java::ArrayList<int> >& ranges);

    void calculateNormals();
    void reorientateNormals();

    int doIntersectionInformation() const;

    /**
    Makes `doIntersectionFirstHit` use the given method: builds its
    acceleration structure (single threaded) unless one of the same method
    is already built for the current sizes; `BRUTE_FORCE` releases it. Also
    computes missing triangle normals. Call it before tracing from several
    threads: afterwards, calls with the same method only read. The result of
    the intersection does not depend on the method.

    The structure is a snapshot: the mutators of this class release it, but
    writes through the non const array getters (`getVertexPositions`,
    `getTriangleIndexes`) do not, so call `releaseAccelerationStructure` or
    this method again after them.
    */
    void prepareAccelerationStructure(TriangleMeshAccelerationMethod method);
    void releaseAccelerationStructure();
    /** Current structure, null when the brute force loop is used */
    const TriangleAccelerationStructure* getAccelerationStructure() const;
    /** When a structure built for the current sizes exists: writes what
    `getMinMax` would return, without walking the vertices, and returns
    true */
    bool getPreparedMinMax(double* outMinMax) const;

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);
    void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;

    int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;
    void getBounds(GeometryBounds& result) const override;
    double* getMinMax() override;

    TriangleMeshGroup* exportToTriangleMeshGroup();

    void compact();
    java::String toString() const;

    void removeSelectedVertices();
    void slice(InfinitePlane& p);
};

#endif
