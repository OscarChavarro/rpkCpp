#ifndef __SCENE__
#define __SCENE__

#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"
#include "vsdk/toolkit/skin/MeshSurface.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/FaceBoundsTable.h"
#include "vsdk/toolkit/scene/SceneQuery.h"

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
    void printSceneQuery() const;
    static void printClusterHierarchy(const RpkGeometry *node, int level, int *elementCount);

    // Face table: every patch of the scene, in depth-first order of
    // clusteredRootGeometry, indexed by Patch::getFaceIndex()
    Patch **faceTable;
    int numberOfFaces;
    FaceBoundsTable *faceBounds; // Owned; bounds of the faceTable entries
    SceneAccelerationMethod accelerationMethod; // Index of sceneQuery; WALD2007 unless an option says otherwise
    SceneQuery *sceneQuery; // Owned; built by buildSceneQuery
    SceneQueryWorkspace *sceneQueryWorkspace; // Owned; query scratch of the single-threaded callers
    static void collectFaces(RpkGeometry *geometry, Patch **table, int capacity, int *count, int *conflicts);

public:
    Background *background;
    Camera *camera;
    ArrayList<RpkGeometry *> *geometryList;
    ArrayList<RpkGeometry *> *clusteredGeometryList;
    RpkGeometry *clusteredRootGeometry;

    // The list of all patches in the current scene
    ArrayList<Patch *> *patchList;

    // The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
    ArrayList<Patch *> *lightSourcePatchList;

    Scene();
    ~Scene();

    void print() const;

    /**
    Assigns the dense face indices and builds the face table and the face
    bounds. Called once, after the cluster hierarchy exists and before the
    scene query. A patch reachable from two geometries, or a patch of
    patchList that clusteredRootGeometry does not reach, is a fatal error.
    */
    void buildFaceTable();
    int getNumberOfFaces() const;
    Patch *getFace(int faceIndex) const;
    const FaceBoundsTable *getFaceBounds() const;

    /** The index buildSceneQuery uses; set while the scene is being built */
    void setAccelerationMethod(SceneAccelerationMethod method);
    SceneAccelerationMethod getAccelerationMethod() const;

    /**
    Builds the scene query over the face table with the acceleration method,
    and the scene's workspace for it. Called once, after buildFaceTable.
    */
    void buildSceneQuery();
    const SceneQuery *getSceneQuery() const;
    SceneQueryWorkspace *getSceneQueryWorkspace() const;
};

inline const FaceBoundsTable *
Scene::getFaceBounds() const {
    return faceBounds;
}

inline SceneAccelerationMethod
Scene::getAccelerationMethod() const {
    return accelerationMethod;
}

inline const SceneQuery *
Scene::getSceneQuery() const {
    return sceneQuery;
}

inline SceneQueryWorkspace *
Scene::getSceneQueryWorkspace() const {
    return sceneQueryWorkspace;
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
