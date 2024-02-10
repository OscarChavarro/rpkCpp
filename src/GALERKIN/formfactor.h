/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR__
#define __FORM_FACTOR__

#include "scene/scene.h"
#include "GALERKIN/interaction.h"

extern int facing(Patch *P, Patch *Q);
extern unsigned areaToAreaFormFactor(INTERACTION *link, GeometryListNode *geometryShadowList);

#endif
