#ifndef __TRIANGLE_MESH__
#define __TRIANGLE_MESH__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshIntersectionIndex.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
class SimpleMaterial;
class Image;
class Ray;
class RayHit;
class TriangleMeshGroup;
class InfinitePlane;

class TriangleMesh : public Surface {
private:
    String name;

    ArrayList<double> vertexPositions;
    ArrayList<double> vertexNormals;
    ArrayList<double> vertexBinormals;
    ArrayList<double> vertexTangents;
    ArrayList<double> vertexColors;
    ArrayList<BOOLEAN> vertexSelections;
    ArrayList<double> vertexUvs;

    ArrayList< ArrayList<int> > incidentTrianglesPerVertexArray;

    ArrayList<int> triangleIndexes;
    ArrayList<double> triangleNormals;

    ArrayList<SimpleMaterial*> materials;
    ArrayList<Image*> textures;

    ArrayList< ArrayList<int> > textureRanges;
    ArrayList< ArrayList<int> > materialRanges;

    TriangleMeshIntersectionIndex intersectionTriangleIndex;
    BOOLEAN ownsMaterials;
    // Optional ray acceleration structure, see prepareAccelerationStructure
    TriangleAccelerationStructureSlot accelerationStructure;

    void interpolateTriangleData(int triangleIndex, double u, double v, RayHit* outHit, const Vector3Dd& rayDirection);
    void fillMaterialAndTexture(int triangleIndex, RayHit* outHit);
    BOOLEAN doIntersectionInternal(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);
    BOOLEAN getTrianglePoints(int index, Vector3Dd& p0, Vector3Dd& p1, Vector3Dd& p2) const;
    void fillTypedDetail(const Ray& ray, double t, int face, double u, double v,
        const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2,
        BOOLEAN normalReversed, RayHit* hit) const;

    double* calculateMinMaxPositions();

    void appendVertices(const ArrayList<double>& ev);
    void appendTriangles(const ArrayList<int>& et);

    void simpleTriangleCut(InfinitePlane& p, ArrayList<double>& extraVertices,
                           ArrayList<int>& extraTriangles, int nv, int i,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void halfTriangleCut(InfinitePlane& p, ArrayList<double>& extraVertices,
                         ArrayList<int>& extraTriangles, int nv, int i, int j,
                         const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void doubleTriangleCut(InfinitePlane& p, ArrayList<double>& extraVertices,
                           ArrayList<int>& extraTriangles, int nv, int i, int j,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    TriangleMesh();
    TriangleMesh(const TriangleMesh& other);
    ~TriangleMesh();

    TriangleMesh* clone() const;
    // Mathematical copy; scene-owned materials and textures are not copied.
    virtual Geometry* copyGeometry() const;
    virtual RayFirstHitStatus::Value queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = NULL) const;
    virtual RayFirstHitStatus::Value querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const;
    virtual RayCrossingSupport::Value emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const;

    String getName() const;
    void setName(const String& name);

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

    void setVertexes(const ArrayList<Vertex>& vertexes, BOOLEAN withNormals, BOOLEAN withBinormals, BOOLEAN withTangents, BOOLEAN withUvs);

    void initTriangleArrays(int n);
    void setTriangles(const ArrayList<Triangle>& triangles);

    void setTextures(const ArrayList<Image*>& textures);
    void setMaterials(const ArrayList<SimpleMaterial*>& materials);
    void setOwnsMaterials(BOOLEAN owns);

    void setVertexAt(int i, const Vertex& vertex);
    void setTriangleAt(int i, const Triangle& triangle);

    int getNumVertices() const;
    int getNumTriangles() const;

    ArrayList<BOOLEAN>& getVertexSelections();

    ArrayList<double>& getVertexPositions();
    const ArrayList<double>& getVertexPositionsView() const { return vertexPositions; }
    ArrayList<double>& getVertexNormals();
    ArrayList<double>& getVertexBinormals();
    ArrayList<double>& getVertexTangents();
    ArrayList<double>& getVertexColors();
    ArrayList<double>& getVertexUvs();
    ArrayList<int>& getTriangleIndexes();
    ArrayList<double>& getTriangleNormals();
    const ArrayList<int>& getTriangleIndexesView() const { return triangleIndexes; }
    const ArrayList<double>& getTriangleNormalsView() const { return triangleNormals; }

    ArrayList<SimpleMaterial*>& getMaterials();
    ArrayList<Image*>& getTextures();
    Image* getTextureAt(int index) const;
    void setTextureAt(int index, Image* image);

    ArrayList< ArrayList<int> >& getTextureRanges();
    ArrayList<int> getTextureRangeAt(int spanRange) const;
    void setTextureRanges(const ArrayList< ArrayList<int> >& ranges);

    ArrayList< ArrayList<int> >& getMaterialRanges();
    ArrayList<int> getMaterialRangeAt(int spanRange) const;
    void setMaterialRanges(const ArrayList< ArrayList<int> >& ranges);

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
    void prepareAccelerationStructure(TriangleMeshAccelerationMethod::Value method);
    void releaseAccelerationStructure();
    /** Current structure, null when the brute force loop is used */
    const TriangleAccelerationStructure* getAccelerationStructure() const;
    /** When a structure built for the current sizes exists: writes what
    `getMinMax` would return, without walking the vertices, and returns
    true */
    BOOLEAN getPreparedMinMax(double* outMinMax) const;

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
    virtual void getBounds(GeometryBounds& result) const;
    virtual double* getMinMax();

    TriangleMeshGroup* exportToTriangleMeshGroup();

    void compact();
    String toString() const;

    void removeSelectedVertices();
    void slice(InfinitePlane& p);
};

#endif
