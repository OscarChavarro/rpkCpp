/* compound.h: COMPOUND geometries. A COMPOUND is basically a list of
 * GEOMetries, useful for representing a scene in a hierarchical manner. */
#ifndef _COMPOUND_H_
#define _COMPOUND_H_

#include <cstdio>

#include "skin/geomlist.h"

/* The specific data for a COMPOUND GEOMetry (see geom.h) is
 * nothing more than the GEOMLIST containing the children GEOMs. */
#define COMPOUND GEOMLIST

/* This function creates a COMPOUND from a linear list of GEOMetries. 
 * Actually, it just counts the number of COMOUNDS in the scene and
 * returns the geomlist. You can as well directly pass 'geomlist'
 * to GeomCreate() for creating a COMPOUND GEOM if you don't want
 * it to be counted. */
extern COMPOUND *CompoundCreate(GEOMLIST *geomlist);

/* A set of pointers to the functions (methods) to operate
 * on COMPOUNDs. */
extern GEOM_METHODS compoundMethods;

#define CompoundMethods() (&compoundMethods)

/* Tests if a GEOM is a compound. */
#define GeomIsCompound(geom) (geom->methods == &compoundMethods)

#endif
