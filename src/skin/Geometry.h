#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "java/util/ArrayList.h"
#include "material/RayHit.h"
#include "skin/BoundingBox.h"
#include "skin/GeometryClassId.h"

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

class PatchSet;
class MeshSurface;
class Compound;
class Element;

class Geometry {
  public: // Will become protected
    static int nextGeometryId;
    static Geometry *excludedGeometry1;
    static Geometry *excludedGeometry2;

    Geometry(
        PatchSet *inPatchSetData,
        Compound *inCompoundData,
        GeometryClassId inClassName);

    bool
    discretizationIntersectPreTest(
        const Ray *ray,
        float minimumDistance,
        const float *maximumDistance
    ) const;

  //public:
    int id; // Unique ID number
    BoundingBox boundingBox;
    Element *radianceData; // Data specific to the radiance algorithm being used
    int itemCount;
    char bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    char omit; // Indicates that the Geometry should not be considered for a number of things,
               // such as intersection testing. Set to false by default, don't forget
               // to set to false again after you changed it!
    bool isDuplicate;

    GeometryClassId className;
    PatchSet *patchSetData;
    Compound *compoundData;

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
    BoundingBox getBoundingBox() const;
    virtual Geometry *duplicateIfPatchSet() const;
};

extern Geometry *geomCreatePatchSet(const java::ArrayList<Patch *> *patchList);
extern void geomDestroy(Geometry *geometry);
extern java::ArrayList<Geometry *> *geomPrimListCopy(const Geometry *geometry);
extern java::ArrayList<Patch *> *geomPatchArrayListReference(Geometry *geometry);
extern void geomDontIntersect(Geometry *geometry1, Geometry *geometry2);
extern void geometryListBounds(const java::ArrayList<Geometry *> *geometryList, BoundingBox *boundingBox);

extern RayHit *
geometryListDiscretizationIntersect(
    const java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

#include "skin/PatchSet.h"
#include "skin/MeshSurface.h"
#include "skin/Compound.h"
#include "skin/Element.h"

#endif
