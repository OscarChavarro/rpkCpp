#ifndef _GEOM_H_
#define _GEOM_H_

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
    void *obj; // specific data for the geometry: varies according to the type of geometry
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
    char flag2; // to make a multiple of 4 bytes
};

/* This function is used to create a new GEOMetry with given specific data and
 * methods. A pointer to the new GEOMetry is returned. */
extern GEOM *GeomCreate(void *obj, GEOM_METHODS *methods);

/* This function prints the GEOMetry data to the file out */
extern void GeomPrint(FILE *out, GEOM *geom);

/* This function returns a bounding box for the GEOMetry */
extern float *GeomBounds(GEOM *geom);

/* This function destroys the given GEOMetry */
extern void GeomDestroy(GEOM *geom);

/* This function returns nonzero if the given GEOMetry is an aggregate. An
 * aggregate is a geometry that consists of simpler geometries. Currently,
 * there is only one type of aggregate geometry: the compound, which is basically 
 * just a list of simpler geometries. Other aggregate geometries are also
 * possible, e.g. CSG objects. If the given GEOMetry is a primitive, zero is 
 * returned. A primitive GEOMetry is a GEOMetry that does not consist of
 * simpler GEOMetries. */
extern int GeomIsAggregate(GEOM *geom);

/* Returns a linear list of the simpler GEOMEtries making up an aggregate GEOMetry.
 * A nullptr pointer is returned if the GEOMetry is a primitive. */
extern GEOMLIST *GeomPrimList(GEOM *geom);

/* Returns a linear list of patches making up a primitive GEOMetry. A nullptr
 * pointer is returned if the given GEOMetry is an aggregate. */
extern PATCHLIST *GeomPatchList(GEOM *geom);

/* This routine returns nullptr is the ray doesn't hit the discretisation of the
 * GEOMetry. If the ray hits the discretisation of the GEOM, containing
 * (among other information) the hit patch is returned.
 * The hitflags (defined in ray.h) determine whether the nearest intersection
 * is returned, or rather just any intersection (e.g. for shadow rays in 
 * ray tracing or for form factor rays in radiosity), whether to consider
 * intersections with front/back facing patches and what other information
 * besides the hit patch (interpolated normal, intersection point, material 
 * properties) to return. 
 * The argument hitstore points to a HITREC in which the hit data can be
 * filled in. */
extern HITREC *GeomDiscretisationIntersect(GEOM *geom, Ray *ray,
                                                  float mindist, float *maxdist,
                                                  int hitflags, HITREC *hitstore);

/* similar, but returns a doubly linked list with all encountered intersections. */
extern HITLIST *GeomAllDiscretisationIntersections(HITLIST *hits,
                                                   GEOM *geom, Ray *ray,
                                                   float mindist, float maxdist,
                                                   int hitflags);


/* Will avoid intersection testing with geom1 and geom2 (possibly nullptr 
 * pointers). Can be used for avoiding immediate selfintersections. */
extern void GeomDontIntersect(GEOM *geom1, GEOM *geom2);

/* This routine creates and returns a duplicate of the given geometry. Needed for
 * shaft culling. */
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
