#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "io/context/MgfParseSession.h"
#include "io/mgf/MgfVertexContext.h"

extern int mgfEntitySphere(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityTorus(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityCylinder(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityRing(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityCone(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityPrism(int ac, const char **av, MgfParseSession *context);
extern int mgfEntityFaceWithHoles(int ac, const char **av, MgfParseSession *context);

#endif
