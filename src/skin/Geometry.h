#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "java/util/ArrayList.h"
#include "skin/bounds.h"
#include "material/hit.h"

/**
Currently, there are three types of geometries:
- the GeometryListNode compound: an aggregate geometry which is basically a list of other
  geometries, useful for representing the scene in a hierarchical manner
  see compound.
- the MeshSurface: a primitive geometry which is basically a list of
  PATCHES representing some simple object with given Material properties
  etc.. see surface.
- the PatchSet (PatchSet): a primitive geometry consisting of a list of patches
  without material properties and such. Used during shaft culling only,
  see shaft culling.

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/

class GEOM_METHODS;
class HITLIST;
class PatchSet;
class MeshSurface;
class Compound;
class GeometryListNode;
class Element;

class Geometry {
  public:
    int id; // Unique ID number
    GEOM_METHODS *methods;
    BOUNDINGBOX bounds;
    Element *radiance_data; // Data specific to the radiance algorithm being used
    int displayListId; // Display list ID for faster hardware rendering - initialised to -1

    // Temporary data
    union {
        int i;
        void *p;
    } tmp;

    // A flag indicating if the geometry has a bounding box
    char bounded; // Non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    char omit; // Indicates that the Geometry should not be considered for a number of things,
               // such as intersection testing. Set to false by default, don't forget
               // to set to false again after you changed it!

    MeshSurface *surfaceData;
    Compound *compoundData;
    PatchSet *patchSetData;
    java::ArrayList<Patch *> *newPatchSetData;
    GeometryListNode *aggregateData;

    int geomCountItems();
};

extern Geometry *geomCreateSurface(MeshSurface *surfaceData, GEOM_METHODS *methods);
extern Geometry *geomCreatePatchSetNew(java::ArrayList<Patch *> *geometryList, GEOM_METHODS *methods);
extern Geometry *geomCreatePatchSet(PatchSet *patchSet, GEOM_METHODS *methods);
extern Geometry *geomCreateCompound(Compound *compoundData, GEOM_METHODS *methods);
extern Geometry *geomCreateAggregateCompound(GeometryListNode *aggregateData, GEOM_METHODS *methods);

extern void geomPrint(FILE *out, Geometry *geom);
extern float *geomBounds(Geometry *geom);
extern void geomDestroy(Geometry *geom);
extern int geomIsAggregate(Geometry *geom);
extern GeometryListNode *geomPrimList(Geometry *geom);
extern PatchSet *geomPatchList(Geometry *geom);
java::ArrayList<Patch *> *geomPatchArrayList(Geometry *geom);
extern void geomDontIntersect(Geometry *geom1, Geometry *geom2);
extern Geometry *geomDuplicate(Geometry *geom);

extern RayHit *
geomDiscretizationIntersect(
        Geometry *geom,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

extern HITLIST *geomAllDiscretizationIntersections(
    HITLIST *hits,
    Geometry *geom,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

/**
Contains pointers to functions (methods) to operate on a
geometry. The 'void *genericAttachedData' passed to the functions is the "state" data
for the geometry, cast to a 'void *'. The implementation of these methods
varies according to the type of geometry. For Compound geometries, the
methods are implemented in compound.c. For surfaces, the methods are
implemented in surface.c.
*/
class GEOM_METHODS {
  public:
    /**
     * This method will compute a bounding box for a geometry. The bounding box
     * is filled in bounding box and a pointer to the filled in bounding box
     * returned
     */
    float *(*getBoundingBox)(void *obj, float *boundingBox);

    /**
     * This method will destroy the geometry and it's children geometries if any
     */
    void (*destroy)(void *obj);

    /**
     * This method will printRegularHierarchy the geometry to the file out
     */
    void (*print)(FILE *out, void *obj);

    /**
     * Returns the list of children geometries if the geometry is an aggregate.
     * This method is not implemented for primitive geometries
     */
    GeometryListNode *(*getPrimitiveGeometryChildrenList)(void *obj);

    /**
     * Returns the list of patches making up a primitive geometry. This
     * method is not implemented for aggregate geometries
     */
    PatchSet *(*getPatchList)(void *obj);

    /**
     * Similar, but appends all found intersections to the hit list hit record
     * list. The possibly modified hit list is returned
     */
    HITLIST *
    (*allDiscretizationIntersections)(
        HITLIST *hits,
        void *obj,
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags);

    /**
     * Duplicate: returns a duplicate of the object's data
     */
    void *(*duplicate)(void *obj);
};

extern Geometry *GLOBAL_geom_excludedGeom1;
extern Geometry *GLOBAL_geom_excludedGeom2;

#include "skin/hitlist.h"
#include "skin/PatchSet.h"
#include "skin/MeshSurface.h"
#include "skin/Compound.h"
#include "skin/geomlist.h"
#include "skin/Element.h"

#endif
