#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "io/context/MgfParseSession.h"

extern int handleVertexEntity(int ac, const char **av, MgfParseSession * /*context*/);
extern int handleFaceEntity(int argc, const char **argv, MgfParseSession *context);
extern int handleFaceWithHolesEntity(int argc, const char **argv, MgfParseSession *context);
extern int handleSurfaceEntity(int argc, const char **argv, MgfParseSession *context);
extern void initGeometryContextTables(MgfParseSession *context);

#endif
