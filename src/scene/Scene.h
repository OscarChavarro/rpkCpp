#ifndef __SCENE__
#define __SCENE__

#include "scene/Background.h"
#include "scene/VoxelGrid.h"
#include "scene/Camera.h"

class Scene {
  private:
    static const char *printGeometryType(GeometryClassId id);
    static void printSurfaceMesh(const MeshSurface *mesh, int level);
    static void printCompound(const Compound *geometry);
    static void printPatchSet(const PatchSet *patchSet);
    void printGeometries() const;
    void printClusteredGeometries() const;
    void printPatches() const;
    static void printClusterHierarchy(const Geometry *node, int level, int *elementCount);

public:
    Background *background;
    Camera *camera;
    java::ArrayList<Geometry *> *geometryList;
    java::ArrayList<Geometry *> *clusteredGeometryList;
    Geometry *clusteredRootGeometry;
    VoxelGrid *voxelGrid;

    // The list of all patches in the current scene
    java::ArrayList<Patch *> *patchList;

    // The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
    java::ArrayList<Patch *> *lightSourcePatchList;

    Scene();
    ~Scene();

    void print() const;
};

#endif
