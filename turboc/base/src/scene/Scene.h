#ifndef __SCENE__
#define __SCENE__

#include "skin/Compound.h"
#include "environment/geometry/elements/PatchSet.h"
#include "skin/MeshSurface.h"
#include "scene/Background.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"

class Scene {
  private:
    static const char *compoundType;
    static const char *meshSurfaceType;
    static const char *patchSetType;
    static const char *unknownType;

    static const char *printGeometryType(GeometryClassId id);
    static void printSurfaceMesh(const MeshSurface *mesh, int level);
    static void printCompound(const Compound *geometry);
    static void printPatchSet(const PatchSet *patchSet);
    void printGeometries() const;
    void printClusteredGeometries() const;
    void printPatches() const;
    void printVoxelGrid() const;
    static void printClusterHierarchy(const Geometry *node, int level, int *elementCount);

public:
    Background *background;
    Camera *camera;
    ArrayList<Geometry *> *geometryList;
    ArrayList<Geometry *> *clusteredGeometryList;
    Geometry *clusteredRootGeometry;
    VoxelGrid *voxelGrid;

    // The list of all patches in the current scene
    ArrayList<Patch *> *patchList;

    // The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
    ArrayList<Patch *> *lightSourcePatchList;

    Scene();
    ~Scene();

    void print() const;
};

#endif
