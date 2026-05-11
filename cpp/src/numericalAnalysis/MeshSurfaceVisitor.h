#ifndef MESH_SURFACE_VISITOR__
#define MESH_SURFACE_VISITOR__

#include "environment/geometry/elements/Patch.h"
#include "skin/MeshSurface.h"

class MeshSurfaceVisitor {
  private:
    static void surfaceConnectFace(MeshSurface *mesh, Patch *face);

  public:
    static void initializeFacesDefaults(MeshSurface *pSurface);
    static void fillFacesBackPointers(MeshSurface *pSurface);
};

#endif
