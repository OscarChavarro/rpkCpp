#include "scene/Scene.h"

static char *globalCompoundType = (char *)"Compound";
static char *globalMeshSurfaceType = (char *)"MeshSurface";
static char *globalPatchSetType = (char *)"PatchSet";
static char *globalUnknownType = (char *)"<unknown>";

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
    clusteredGeometryList = new java::ArrayList<Geometry *>();
}

Scene::~Scene() {
    if ( lightSourcePatchList != nullptr ) {
        delete lightSourcePatchList;
        lightSourcePatchList = nullptr;
    }
    if ( clusteredGeometryList != nullptr ) {
        delete clusteredGeometryList;
        clusteredGeometryList = nullptr;
    }
    if ( clusteredRootGeometry != nullptr ) {
        // This is deleted on Cluster::deleteCachedGeometries()
        clusteredRootGeometry = nullptr;
    }
    if ( patchList != nullptr ) {
        delete patchList;
        patchList = nullptr;
    }
    if ( voxelGrid != nullptr ) {
        delete voxelGrid;
        voxelGrid = nullptr;
    }
    if ( background != nullptr ) {
        delete background;
        background = nullptr;
    }
    if ( camera != nullptr ) {
        delete camera;
        camera = nullptr;
    }
}

char *
Scene::printGeometryType(GeometryClassId id) {
    char *response = globalUnknownType;
    if ( id == GeometryClassId::SURFACE_MESH ) {
        response = globalMeshSurfaceType;
    } else if ( id == GeometryClassId::COMPOUND ) {
        response = globalCompoundType;
    } else if ( id == GeometryClassId::PATCH_SET ) {
        response = globalPatchSetType;
    }
    return response;
}

void
Scene::printPatchSet(const PatchSet *patchSet) {
    if ( patchSet->patchList != nullptr ) {
        printf("  - Patches: %ld\n", patchSet->patchList->size());
    } else {
        printf("  - Patches: null list!\n");
    }
}

void
Scene::printCompound(const Compound *compound) {
    if ( compound->compoundData->children != nullptr ) {
        printf("    . Outer children: %ld\n", compound->compoundData->children->size());
        for ( int i = 0; i < compound->compoundData->children->size(); i++ ) {
            const Geometry *child = compound->compoundData->children->get(i);
            printf("    . Child [%d] / [%s]\n", i, printGeometryType(child->className));
            if ( child->className == GeometryClassId::SURFACE_MESH ) {
                printSurfaceMesh((MeshSurface *)child, 6);
            }
        }
    }
    if ( compound->children != nullptr ) {
        printf("    . Inner children: %ld\n", compound->children->size());
    }
}

void
Scene::printSurfaceMesh(const MeshSurface *mesh, int level) {
    char *spaces = new char[level + 1];

    int i;
    for ( i = 0; i < level; i++ ) {
        spaces[i] = ' ';
    }
    spaces[i] = '\0';

    printf("%s  - Type: SurfaceMesh\n", spaces);
    printf("%s    . Inner id: %d\n", spaces, mesh->meshId);
    printf("%s    . Vertices: %ld, positions: %ld, normals: %ld, faces: %ld\n",
       spaces, mesh->vertices->size(), mesh->positions->size(), mesh->normals->size(), mesh->faces->size());

    delete[] spaces;
}

void Scene::print() const {
    printf("= geometryList ================================================================\n");
    printf("Geometries on list: %ld\n", geometryList->size());
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        printf("  - Index: [%d of %ld] / [%s]\n", i + 1, geometryList->size(), printGeometryType(geometry->className));
        printf("    . Id: %d\n", geometry->id);
        printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");

        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            printSurfaceMesh((MeshSurface *)geometry, 0);
        } else if ( geometry->className == GeometryClassId::COMPOUND ) {
            printCompound((Compound *)geometry);
        } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
            printPatchSet((PatchSet *)geometry);
        }
    }
}
