#include "scene/Scene.h"

Scene::Scene():
    background(),
    camera(),
    geometryList(),
    clusteredGeometryList(),
    clusteredRootGeometry(),
    voxelGrid(),
    patchList(),
    lightSourcePatchList()
{
    camera = new Camera();
}

Scene::~Scene() {
    delete lightSourcePatchList;
    delete clusteredGeometryList;
    if ( patchList != nullptr ) {
        delete patchList;
    }
    if ( voxelGrid != nullptr ) {
        delete voxelGrid;
        voxelGrid = nullptr;
    }
}
