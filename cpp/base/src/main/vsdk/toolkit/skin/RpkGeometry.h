#ifndef RPK_GEOMETRY__
#define RPK_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
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
  an object with given Material properties
- PatchSet: a primitive geometry consisting of a list of patches

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/

class RpkGeometry {
  private:
    static java::ArrayList<RpkGeometry *> *
    cloneGeometryList(const java::ArrayList<RpkGeometry *> *input);

  public: // Will become protected
    static int nextGeometryId;

    explicit RpkGeometry(GeometryClassId inClassName);

    bool
    discretizationIntersectPreTest(
        const RpkRay *ray,
        float minimumDistance,
        const float *maximumDistance,
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

    virtual RpkRayHit *
    discretizationIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions) const;

    static RpkRayHit *
    patchListIntersect(
        const java::ArrayList<Patch *> *patchList,
        const RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions);

    RpkAxisAlignedBoundingBox getBoundingBox() const;
    RpkGeometry *clone() const;

    static void destroy(RpkGeometry *geometry);
    static java::ArrayList<RpkGeometry *> *primitiveListCopy(const RpkGeometry *geometry);
    static java::ArrayList<Patch *> *patchListReference(const RpkGeometry *geometry);
    static void listBounds(const java::ArrayList<RpkGeometry *> *geometryList, RpkAxisAlignedBoundingBox *boundingBox);

    static RpkRayHit *
    listDiscretizationIntersect(
        const java::ArrayList<RpkGeometry *> *geometryList,
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions);

  protected:
    static RpkAxisAlignedBoundingBox *
    patchListBounds(const java::ArrayList<Patch *> *patchList, RpkAxisAlignedBoundingBox *boundingBox);
};

#endif
