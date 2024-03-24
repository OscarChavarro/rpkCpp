#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "io/mgf/MgfVertexContext.h"
#include "io/mgf/MgfContext.h"

extern char *GLOBAL_mgf_currentVertexName;
extern int GLOBAL_mgf_divisionsPerQuarterCircle;
extern MgfVertexContext GLOBAL_mgf_vertexContext;

extern int mgfEntitySphere(int ac, char **av, MgfContext *context);
extern int mgfEntityTorus(int ac, char **av, MgfContext *context);
extern int mgfEntityCylinder(int ac, char **av, MgfContext *context);
extern int mgfEntityRing(int ac, char **av, MgfContext *context);
extern int mgfEntityCone(int ac, char **av, MgfContext *context);
extern int mgfEntityPrism(int ac, char **av, MgfContext *context);
extern int mgfEntityFaceWithHoles(int ac, char **av, MgfContext *context);

#endif
