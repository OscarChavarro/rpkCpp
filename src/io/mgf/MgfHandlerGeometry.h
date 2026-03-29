#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "io/context/BaseContext.h"

extern int handleVertexEntity(int ac, const char **av, BaseContext * /*context*/);
extern int handleFaceEntity(int argc, const char **argv, BaseContext *context);
extern int handleFaceWithHolesEntity(int argc, const char **argv, BaseContext *context);
extern int handleSurfaceEntity(int argc, const char **argv, BaseContext *context);
extern void initGeometryContextTables(BaseContext *context);

#endif
