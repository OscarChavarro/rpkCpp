#ifndef GEOMETRY__
#define GEOMETRY__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Ray.h"
#include "environment/geometry/elements/RayHit.h"
#include "skin/AxisAlignedBoundingBox.h"
#include "environment/geometry/elements/Element.h"
#include "skin/GeometryClassId.h"
#include "skin/MinMaxBox.h"
#include "environment/geometry/elements/Patch.h"

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
    static java::ArrayList<Geometry *> *
    cloneGeometryList(const java::ArrayList<Geometry *> *input);

  public: // Will become protected
    static int nextGeometryId;
    static Geometry *excludedGeometry1;
    static Geometry *excludedGeometry2;

    explicit Geometry(GeometryClassId inClassName);

    bool
    discretizationIntersectPreTest(
        const Ray *ray,
        float minimumDistance,
        const float *maximumDistance
    ) const;

  //public:
    int id; // Unique ID number
    AxisAlignedBoundingBox boundingBox;
    mutable MinMaxBox *rayIntersectionBox;
    Element *radianceData; // Data specific to the radiance algorithm being used
    int itemCount;
    char bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    char omit; // Indicates that the Geometry should not be considered for a number of things,
               // such as intersection testing. Set to false by default, don't forget
               // to set to false again after you changed it!
    bool isDuplicate;

    GeometryClassId className;

    Geometry();
    virtual ~Geometry();

    bool isCompound() const;

    virtual RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const;

    static RayHit *
    patchListIntersect(
        const java::ArrayList<Patch *> *patchList,
        const Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

    bool isExcluded() const;
    AxisAlignedBoundingBox getBoundingBox() const;
    MinMaxBox *getRayIntersectionBox() const;
    Geometry *clone() const;

    static void destroy(Geometry *geometry);
    static java::ArrayList<Geometry *> *primitiveListCopy(const Geometry *geometry);
    static java::ArrayList<Patch *> *patchListReference(const Geometry *geometry);
    static void dontIntersect(Geometry *geometry1, Geometry *geometry2);
    static void listBounds(const java::ArrayList<Geometry *> *geometryList, AxisAlignedBoundingBox *boundingBox);

    static RayHit *
    listDiscretizationIntersect(
        const java::ArrayList<Geometry *> *geometryList,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

  protected:
    static AxisAlignedBoundingBox *
    patchListBounds(const java::ArrayList<Patch *> *patchList, AxisAlignedBoundingBox *boundingBox);
};

#endif
