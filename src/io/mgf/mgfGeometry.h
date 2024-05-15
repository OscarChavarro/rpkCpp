#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "io/mgf/MgfVertexContext.h"
#include "io/mgf/MgfContext.h"

extern int mgfEntitySphere(int ac, const char **av, MgfContext *context);
extern int mgfEntityTorus(int ac, const char **av, MgfContext *context);
extern int mgfEntityCylinder(int ac, const char **av, MgfContext *context);
extern int mgfEntityRing(int ac, const char **av, MgfContext *context);
extern int mgfEntityCone(int ac, const char **av, MgfContext *context);
extern int mgfEntityPrism(int ac, const char **av, MgfContext *context);
extern int mgfEntityFaceWithHoles(int ac, const char **av, MgfContext *context);

#endif
