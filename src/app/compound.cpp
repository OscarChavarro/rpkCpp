#include <cstdio>

#include "material/statistics.h"
#include "skin/geom.h"
#include "app/compound.h"

/* "creates" a COMPOUND from a list of GEOMetries */
COMPOUND *CompoundCreate(GEOMLIST *geomlist) {
    GLOBAL_statistics_numberOfCompounds++;
    return geomlist;
}

/* This method will compute a bounding box for a GEOMetry. The bounding box
 * is filled in in boundingbox and a pointer to the filled in boundingbox returned. */
static float *CompoundBounds(COMPOUND *obj, float *boundingbox) {
    return GeomListBounds(obj, boundingbox);
}

/* this method will destroy the GEOMetry and it's children GEOMetries if any */
static void CompoundDestroy(COMPOUND *obj) {
    GeomListIterate(obj, GeomDestroy);
    GeomListDestroy(obj);
    GLOBAL_statistics_numberOfCompounds--;
}

/* this method will print the GEOMetry to the file out */
void CompoundPrint(FILE *out, COMPOUND *obj) {
    fprintf(out, "compound\n");
    GeomListIterate1B(obj, GeomPrint, out);
    fprintf(out, "end of compound\n");
}

/* returns the list of children GEOMetries if the GEOM is an aggregate */
GEOMLIST *CompoundPrimitives(COMPOUND *obj) {
    return obj;
}

HITREC *CompoundDiscretisationIntersect(COMPOUND *obj, Ray *ray, float mindist, float *maxdist, int hitflags,
                                               HITREC *hitstore) {
    return GeomListDiscretisationIntersect(obj, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *
CompoundAllDiscretisationIntersections(HITLIST *hits, COMPOUND *obj, Ray *ray, float mindist, float maxdist,
                                       int hitflags) {
    return GeomListAllDiscretisationIntersections(hits, obj, ray, mindist, maxdist, hitflags);
}

GEOM_METHODS compoundMethods = {
    (float *(*)(void *, float *)) CompoundBounds,
    (void (*)(void *)) CompoundDestroy,
    (void (*)(FILE *, void *)) CompoundPrint,
    (GEOMLIST *(*)(void *)) CompoundPrimitives,
    (PATCHLIST *(*)(void *)) nullptr,
    (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) CompoundDiscretisationIntersect,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) CompoundAllDiscretisationIntersections,
    (void *(*)(void *)) nullptr
};
