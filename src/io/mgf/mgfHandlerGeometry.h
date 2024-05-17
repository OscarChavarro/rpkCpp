#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "io/mgf/MgfContext.h"

extern int handleVertexEntity(int ac, const char **av, MgfContext * /*context*/);
extern int handleFaceEntity(int argc, const char **argv, MgfContext *context);
extern int handleFaceWithHolesEntity(int argc, const char **argv, MgfContext *context);
extern int handleSurfaceEntity(int argc, const char **argv, MgfContext *context);
extern void initGeometryContextTables(MgfContext *context);

#endif
