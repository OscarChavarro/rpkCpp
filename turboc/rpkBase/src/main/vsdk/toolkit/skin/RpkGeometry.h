#ifndef __RPK_GEOMETRY__
#define __RPK_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/skin/GeometryClassId.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
Currently, there are three types of geometries:
- Compound: an aggregate geometry which is basically a list of other
  geometries, useful for representing the scene in a hierarchical manner
- MeshSurface: a primitive geometry which is a list of patches representing
  an object with given RpkMaterial properties
- PatchSet: a primitive geometry consisting of a list of patches

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/

class RpkGeometry {
  private:
    // Interval [firstFaceIndex, lastFaceIndex] of Patch::getFaceIndex()
    // values of the patches below this geometry. Scene::buildFaceTable()
    // numbers the faces depth-first over the cluster hierarchy, so every
    // geometry of that hierarchy covers a contiguous interval. Empty
    // (first > last) until the face table is built.
    int firstFaceIndex;
    int lastFaceIndex;

    static ArrayList<RpkGeometry *> *
    cloneGeometryList(const ArrayList<RpkGeometry *> *input);

  public: // Will become protected
    static int nextGeometryId;

    explicit RpkGeometry(GeometryClassId inClassName);

    bool
    discretizationIntersectPreTest(
        const Ray *ray,
        double minimumDistance,
        const double *maximumDistance,
        const RayExclusionSet &exclusions
    ) const;

  //public:
    int id; // Unique ID number
    RpkAxisAlignedBoundingBox boundingBox;
    Element *radianceData; // Data specific to the radiance algorithm being used
    int itemCount;
    char bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    bool isDuplicate;

    GeometryClassId className;

    RpkGeometry();
    virtual ~RpkGeometry();

    bool isCompound() const;
    int getFirstFaceIndex() const;
    int getLastFaceIndex() const;
    void setFaceIndexRange(int first, int last);

    virtual RpkRayHit *
    discretizationIntersect(
        Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions) const;

    static RpkRayHit *
    patchListIntersect(
        const ArrayList<Patch *> *patchList,
        const Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions);

    RpkAxisAlignedBoundingBox getBoundingBox() const;
    RpkGeometry *clone() const;

    static void destroy(RpkGeometry *geometry);
    static ArrayList<RpkGeometry *> *primitiveListCopy(const RpkGeometry *geometry);
    static ArrayList<Patch *> *patchListReference(const RpkGeometry *geometry);
    static void listBounds(const ArrayList<RpkGeometry *> *geometryList, RpkAxisAlignedBoundingBox *boundingBox);

    static RpkRayHit *
    listDiscretizationIntersect(
        const ArrayList<RpkGeometry *> *geometryList,
        Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions);

  protected:
    static RpkAxisAlignedBoundingBox *
    patchListBounds(const ArrayList<Patch *> *patchList, RpkAxisAlignedBoundingBox *boundingBox);
};

#endif
