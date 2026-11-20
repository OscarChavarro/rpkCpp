#ifndef SCENE__
#define SCENE__

#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"
#include "vsdk/toolkit/skin/MeshSurface.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/FaceBoundsTable.h"
#include "vsdk/toolkit/scene/DirectPotentialTable.h"
#include "vsdk/toolkit/scene/FaceVisibilityTable.h"
#include "vsdk/toolkit/scene/FaceColorTable.h"
#include "vsdk/toolkit/scene/RadianceElementTable.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"
#include "vsdk/toolkit/scene/VisibilityQueryRecorder.h"

class Scene {
  private:
    void deleteRadianceElements();
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
    static void printClusterHierarchy(const RpkGeometry *node, int level, int *elementCount);

    // Face table: every patch of the scene, in depth-first order of
    // clusteredRootGeometry, indexed by Patch::getFaceIndex(). Owned array,
    // patches not owned. Each geometry and cluster covers a contiguous range.
    Patch **faceTable;
    int numberOfFaces;
    FaceBoundsTable *faceBounds; // Owned; bounds of the faceTable entries
    DirectPotentialTable *directPotentials; // Owned; direct potential of the faceTable entries
    FaceVisibilityTable *faceVisibility; // Owned; direct visibility of the faceTable entries
    FaceColorTable *faceColors; // Owned; display color of the faceTable entries
    RadianceElementTable *radianceElements; // Owned table; top-level radiance element of the faceTable entries
    static void collectFaces(const RpkGeometry *geometry, Patch **table, int capacity, int *count, int *conflicts);

public:
    Background *background;
    Camera *camera;
    java::ArrayList<RpkGeometry *> *geometryList;
    java::ArrayList<RpkGeometry *> *clusteredGeometryList;
    RpkGeometry *clusteredRootGeometry;
    VoxelGrid *voxelGrid;
    VoxelGridWorkspace *voxelGridWorkspace; // Query scratch for voxelGrid, owned by the scene
    VisibilityQueryRecorder *visibilityQueryRecorder; // Ray capture builds only, otherwise null

    // The list of all patches in the current scene
    java::ArrayList<Patch *> *patchList;

    // The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
    java::ArrayList<Patch *> *lightSourcePatchList;

    Scene();
    ~Scene();

    void print() const;

    /**
    Assigns the dense face indices and builds the face table and the face
    bounds. Called once, after the cluster hierarchy exists and before the
    voxel grid. A patch reachable from two geometries, or a patch of
    patchList that clusteredRootGeometry does not reach, is a fatal error.
    */
    void buildFaceTable();
    int getNumberOfFaces() const;
    Patch *getFace(int faceIndex) const;
    const FaceBoundsTable *getFaceBounds() const;
    // Per-face solver data: written by Potential::updateDirectPotential, read
    // by the importance-driven solvers
    DirectPotentialTable *getDirectPotentials() const;
    // Written by Potential::updateDirectVisibility, read by the importance-driven
    // stochastic radiosity
    FaceVisibilityTable *getFaceVisibility() const;
    // Written by the radiance methods after each step, read by the displays
    FaceColorTable *getFaceColors() const;
    // Filled by Radiance::setRadianceMethod, read by the radiance methods
    RadianceElementTable *getRadianceElements() const;
};

inline RadianceElementTable *
Scene::getRadianceElements() const {
    return radianceElements;
}

inline FaceColorTable *
Scene::getFaceColors() const {
    return faceColors;
}

inline FaceVisibilityTable *
Scene::getFaceVisibility() const {
    return faceVisibility;
}

inline DirectPotentialTable *
Scene::getDirectPotentials() const {
    return directPotentials;
}

inline const FaceBoundsTable *
Scene::getFaceBounds() const {
    return faceBounds;
}

inline int
Scene::getNumberOfFaces() const {
    return numberOfFaces;
}

inline Patch *
Scene::getFace(const int faceIndex) const {
    return faceTable[faceIndex];
}

#endif
