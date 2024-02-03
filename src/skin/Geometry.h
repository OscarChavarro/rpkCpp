#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "java/util/ArrayList.h"
#include "skin/bounds.h"
#include "skin/hitlist.h"

/**
Currently, there are three types of geometries:
- the Compound: an aggregate geometry which is basically a list of other
  geometries, useful for representing the scene in a hierarchical manner
  see compound.
- the MeshSurface: a primitive geometry which is basically a list of
  PATCHES representing some simple object with given Material properties
  etc. see surface.
- the PatchSet: a primitive geometry consisting of a list of patches
  without material properties and such. Used during shaft culling only,
  see shaft culling.

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/

class PatchSet;
class MeshSurface;
class Compound;
class GeometryListNode;
class Element;

enum GeometryClassId {
    PATCH_SET,
    SURFACE_MESH,
    COMPOUND,
    UNDEFINED
};

class Geometry {
  public:
    int id; // Unique ID number
    BOUNDINGBOX bounds;
    Element *radiance_data; // Data specific to the radiance algorithm being used
    int displayListId; // Display list ID for faster hardware rendering - initialised to -1

    int itemCount;

    // A flag indicating if the geometry has a bounding box
    char bounded; // Non-zero if bounded geometry
    char shaftCullGeometry; // Generated during shaft culling
    char omit; // Indicates that the Geometry should not be considered for a number of things,
               // such as intersection testing. Set to false by default, don't forget
               // to set to false again after you changed it!

    MeshSurface *surfaceData;
    Compound *compoundData;
    PatchSet *patchSetData;

    Geometry();
    int geomCountItems();
    GeometryClassId className;
};

extern Geometry *geomCreateSurface(MeshSurface *surfaceData);
extern Geometry *geomCreatePatchSet(java::ArrayList<Patch *> *patchList);
extern Geometry *geomCreatePatchSet(PatchSet *patchSet);
extern Geometry *geomCreateCompound(Compound *compoundData);

extern float *geomBounds(Geometry *geometry);
extern void geomDestroy(Geometry *geometry);
extern int geomIsAggregate(Geometry *geometry);
extern GeometryListNode *geomPrimList(Geometry *geometry);
java::ArrayList<Patch *> *geomPatchArrayList(Geometry *geometry);
extern void geomDontIntersect(Geometry *geometry1, Geometry *geometry2);
extern Geometry *geomDuplicate(Geometry *geometry);

extern RayHit *
geomDiscretizationIntersect(
    Geometry *geometry,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

extern HITLIST *
geomAllDiscretizationIntersections(
    HITLIST *hits,
    Geometry *geometry,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

extern Geometry *GLOBAL_geom_excludedGeom1;
extern Geometry *GLOBAL_geom_excludedGeom2;

#include "skin/PatchSet.h"
#include "skin/MeshSurface.h"
#include "skin/Compound.h"
#include "skin/geomlist.h"
#include "skin/Element.h"

#endif
