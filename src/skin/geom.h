#ifndef _GEOM_H_
#define _GEOM_H_

#include "java/util/ArrayList.h"
#include "common/bounds.h"
#include "material/hit.h"

/* Currently, there are three types of GEOMetries:
 *
 * - the COMPOUND: an aggregate GEOMetry which is basically a list of other
 *	GEOMetries, useful for representing the scene in a hierarchical manner
 *	see compound.
 *
 * - the SURFACE: a primitive GEOMetry which is basically a list of
 *	PATCHES representing some simple object with given MATERIAL properties
 *	etc.. see surface.
 *
 * - the PATCHLIST: a primitive GEOMetry consisting of a list of patches
 *	without material properties and such. Used during shaft culling only,
 *	see patchlist.c, patchlist_geom.h, shaftculling.
 *
 * Each of these primitieves has certain specific data. The GEOM class
 * contains data that is independent of GEOMetry type. */

class GEOM_METHODS;
class GEOMLIST;
class HITLIST;
class PATCHLIST;

class GEOM {
  public:
    int id; // Unique ID number
    GEOM_METHODS *methods;
    BOUNDINGBOX bounds;
    void *radiance_data; // data specific to the radiance algorithm being used
    int dlistid; // display list ID for faster hardware rendering - initialised to -1

    // Temporary data
    union {
        int i;
        void *p;
    } tmp;

    // A flag indicating whether or not the geometry has a bounding box
    char bounded; // nonzero if bounded geometry
    char shaftcullgeom; // generated during shaftculling
    char omit; /* indicates that the GEOM should not be considered for a number of things, 
                  such as intersection testing. Set to false by default, don't forget
                  to set to false again after you changed it! */

    java::ArrayList<PATCH *> *newPatchSetData;

    void *obj; // specific data for the geometry: varies according to the type of geometry
};

extern GEOM *GeomCreate(void *obj, GEOM_METHODS *methods);
extern GEOM *geomCreatePatchSetNew(java::ArrayList<PATCH *> *geometryList, GEOM_METHODS *methods);
GEOM *geomCreatePatchSet(PATCHLIST *patchSet, GEOM_METHODS *methods);
extern void GeomPrint(FILE *out, GEOM *geom);
extern float *GeomBounds(GEOM *geom);
extern void GeomDestroy(GEOM *geom);
extern int GeomIsAggregate(GEOM *geom);
extern GEOMLIST *GeomPrimList(GEOM *geom);
extern PATCHLIST *GeomPatchList(GEOM *geom);

extern HITREC *
GeomDiscretisationIntersect(GEOM *geom, Ray *ray,
                                                  float mindist, float *maxdist,
                                                  int hitflags, HITREC *hitstore);

extern HITLIST *GeomAllDiscretisationIntersections(HITLIST *hits,
                                                   GEOM *geom, Ray *ray,
                                                   float mindist, float maxdist,
                                                   int hitflags);


extern void GeomDontIntersect(GEOM *geom1, GEOM *geom2);
extern GEOM *GeomDuplicate(GEOM *geom);

/**
Contains pointers to functions (methods) to operate on a
GEOMetry. The 'void *obj' passed to the functions is the "state" data
for the GEOMetry, cast to a 'void *'. The implementation of these methods
varies according to the type of GEOMetry. For COMPOUND geometries, the
methods are implemented in compound.c. For SURFACEs, the methods are
implemented in surface.c.
*/
class GEOM_METHODS {
  public:
    /* this method will compute a bounding box for a GEOMetry. The bounding box
     * is filled in boundingbox and a pointer to the filled in boundingbox
     * returned. */
    float *(*bounds)(void *obj, float *boundingbox);

    /* this method will destroy the GEOMetry and it's children GEOMetries if
     * any */
    void (*destroy)(void *obj);

    /* this method will print the GEOMetry to the file out */
    void (*print)(FILE *out, void *obj);

    /* returns the list of children GEOMetries if the GEOM is an aggregate.
     * This method is not implemented for primitive GEOMetries. */
    GEOMLIST *(*primlist)(void *obj);

    /* returns the list of PATCHes making up a primitive GEOMetry. This
     * method is not implemented for aggregate GEOMetries. */
    PATCHLIST *(*patchlist)(void *obj);

    /* discretisation_intersect returns nullptr is the ray doesn't hit the discretisation
     * of the object. If the ray hits the object, a hit record is returned containing
     * information about the intersection point. See geom.h for more explanation. */
    HITREC *
    (*discretisation_intersect)(void *obj, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

    /* similar, but appends all found intersections to the hitlist hit record
     * list. The possibly modified hitlist is returned. */
    HITLIST *
    (*all_discretisation_intersections)(HITLIST *hits, void *obj, Ray *ray, float mindist, float maxdist,
                                        int hitflags);

    /* duplicate: returns a duplicate of the object's data */
    void *(*duplicate)(void *obj);
};

extern GEOM *excludedGeom1;
extern GEOM *excludedGeom2;

#include "skin/geomlist.h"
#include "skin/hitlist.h"
#include "skin/patchlist.h"

#endif
