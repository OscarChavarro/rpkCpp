#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHit.h"
#include "vsdk/toolkit/skin/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/skin/GeometryClassId.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
Currently, there are three types of geometries:
- Compound: an aggregate geometry which is basically a list of other
  geometries, useful for representing the scene in a hierarchical manner
- MeshSurface: a primitive geometry which is a list of patches representing
  an object with given Material properties
- PatchSet: a primitive geometry consisting of a list of patches

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/

class Geometry {
  private:
    // Interval [firstFaceIndex, lastFaceIndex] of Patch::getFaceIndex()
    // values of the patches below this geometry. Scene::buildFaceTable()
    // numbers the faces depth-first over the cluster hierarchy, so every
    // geometry of that hierarchy covers a contiguous interval. Empty
    // (first > last) until the face table is built.
    int firstFaceIndex;
    int lastFaceIndex;

    static ArrayList<Geometry *> *
    cloneGeometryList(const ArrayList<Geometry *> *input);

  public: // Will become protected
    static int nextGeometryId;

    explicit Geometry(GeometryClassId inClassName);

    bool
    discretizationIntersectPreTest(
        const RpkRay *ray,
        float minimumDistance,
        const float *maximumDistance,
        const RayExclusionSet &exclusions
    ) const;

  //public:
    int id; // Unique ID number
    BoundingBox boundingBox;
    Element *radianceData; // Data specific to the radiance algorithm being used
    int itemCount;
    char bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    bool isDuplicate;

    GeometryClassId className;

    Geometry();
    virtual ~Geometry();

    bool isCompound() const;
    int getFirstFaceIndex() const;
    int getLastFaceIndex() const;
    void setFaceIndexRange(int first, int last);

    virtual RayHit *
    discretizationIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore,
        const RayExclusionSet &exclusions) const;

    static RayHit *
    patchListIntersect(
        const ArrayList<Patch *> *patchList,
        const RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore,
        const RayExclusionSet &exclusions);

    BoundingBox getBoundingBox() const;
    Geometry *clone() const;

    static void destroy(Geometry *geometry);
    static ArrayList<Geometry *> *primitiveListCopy(const Geometry *geometry);
    static ArrayList<Patch *> *patchListReference(const Geometry *geometry);
    static void listBounds(const ArrayList<Geometry *> *geometryList, BoundingBox *boundingBox);

    static RayHit *
    listDiscretizationIntersect(
        const ArrayList<Geometry *> *geometryList,
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore,
        const RayExclusionSet &exclusions);

  protected:
    static BoundingBox *
    patchListBounds(const ArrayList<Patch *> *patchList, BoundingBox *boundingBox);
};

#endif
