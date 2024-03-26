#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

extern int GLOBAL_mgf_inComplex;

extern int handleVertexEntity(int ac, char **av, MgfContext * /*context*/);
extern int handleFaceEntity(int argc, char **argv, MgfContext *context);
extern int handleFaceWithHolesEntity(int argc, char **argv, MgfContext *context);
extern int handleSurfaceEntity(int argc, char **argv, MgfContext *context);
extern void initGeometryContextTables(MgfContext *context);

#endif
