#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "skin/RadianceMethod.h"
#include "io/mgf/MgfVertexContext.h"

extern char *GLOBAL_mgf_currentVertexName;
extern MgfVertexContext *GLOBAL_mgf_currentVertex;
extern MgfVertexContext GLOBAL_mgf_vertexContext;

extern int mgfEntitySphere(int ac, char **av, RadianceMethod *context);
extern int mgfEntityTorus(int ac, char **av, RadianceMethod *context);
extern int mgfEntityCylinder(int ac, char **av, RadianceMethod *context);
extern int mgfEntityRing(int ac, char **av, RadianceMethod *context);
extern int mgfEntityCone(int ac, char **av, RadianceMethod *context);
extern int mgfEntityPrism(int ac, char **av, RadianceMethod *context);
extern int mgfEntityFaceWithHoles(int ac, char **av, RadianceMethod *context);

#endif
