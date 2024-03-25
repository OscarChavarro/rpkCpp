#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/mgf/MgfContext.h"

extern java::ArrayList<Vector3D *> *GLOBAL_mgf_currentPointList;
extern java::ArrayList<Vector3D *> *GLOBAL_mgf_currentNormalList;
extern java::ArrayList<Vertex *> *GLOBAL_mgf_currentVertexList;
extern java::ArrayList<Patch *> *GLOBAL_mgf_currentFaceList;
extern java::ArrayList<Geometry *> *GLOBAL_mgf_currentGeometryList;
extern int GLOBAL_mgf_inSurface;

extern int handleObjectEntity(int argc, char **argv, MgfContext * /*context*/);
extern void newSurface();
extern void surfaceDone(MgfContext *context);

#endif
