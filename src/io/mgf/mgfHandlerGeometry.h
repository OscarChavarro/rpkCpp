#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

extern bool GLOBAL_mgf_allSurfacesSided;
extern int GLOBAL_mgf_inComplex;

extern int handleFaceEntity(int argc, char **argv, RadianceMethod *context);
extern int handleFaceWithHolesEntity(int argc, char **argv, RadianceMethod *context);
extern int handleSurfaceEntity(int argc, char **argv, RadianceMethod *context);

#endif
