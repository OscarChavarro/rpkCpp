#ifndef __MESH_SURFACE_VISITOR__
#define __MESH_SURFACE_VISITOR__

#include "skin/Patch.h"

class MeshSurfaceVisitor {
public:
    static void surfaceConnectFace(MeshSurface *mesh, Patch *face);

    static void fillFacesBackPointers(MeshSurface *pSurface);
};

#endif
