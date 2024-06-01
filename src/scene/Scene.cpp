#include "java/util/ArrayList.txx"
#include "scene/Scene.h"

static const char *globalCompoundType = "Compound";
static const char *globalMeshSurfaceType = "MeshSurface";
static const char *globalPatchSetType = "PatchSet";
static const char *globalUnknownType = "<unknown>";

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

const char *
Scene::printGeometryType(GeometryClassId id) {
    const char *response = globalUnknownType;
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
    if ( patchSet->getPatchList() != nullptr ) {
        printf("  - Patches: %ld\n", patchSet->getPatchList()->size());
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
                printSurfaceMesh((const MeshSurface *)child, 6);
            }
        }
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

    printf("%s  - Object name: %s\n", spaces, mesh->objectName);
    printf("%s  - Type: SurfaceMesh\n", spaces);
    printf("%s    . Inner id: %d\n", spaces, mesh->meshId);
    printf("%s    . Vertices: %ld, positions: %ld, normals: %ld, faces: %ld\n",
       spaces, mesh->vertices->size(), mesh->positions->size(), mesh->normals->size(), mesh->faces->size());

    delete[] spaces;
}

void
Scene::printGeometries() const {
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

void
Scene::printClusteredGeometries() const {
    printf("= clusteredGeometryList ================================================================\n");
    printf("Geometry clusters on list: %ld\n", clusteredGeometryList->size());
    for ( int i = 0; i < clusteredGeometryList->size(); i++ ) {
        Geometry *geometry = clusteredGeometryList->get(i);
        printf("  - Index: [%d of %ld] / [%s]\n", i + 1, clusteredGeometryList->size(), printGeometryType(geometry->className));
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

void
Scene::printPatches() const {
    printf("= patchList ================================================================\n");
    if ( patchList == nullptr ) {
        printf("Patches on top level scene list: NULL\n");
        return;
    }
    printf("Patches on top level scene list: %ld\n", patchList->size());
    for ( int i = 0; i < patchList->size(); i++ ) {
        const Patch *patch = patchList->get(i);
        printf("  - patch[%d]: vertices: %d, area: %03f\n",
           i, patch->numberOfVertices, patch->area);
    }
}

void
Scene::printClusterHierarchy(const Geometry *node, int level, int *elementCount) {
    if ( level == 0 ) {
        printf("= clusteredRootGeometry ================================================================\n");
    }
    switch ( level ) {
        case 0:
            break;
        case 1:
            printf("* ");
            break;
        case 2:
            printf("  - ");
            break;
        case 3:
            printf("    . ");
            break;
        default:
            printf("   ");
            for ( int j = 0; j < level; j++ ) {
                printf(" ");
            }
            printf("[%d] ", level);
            break;
    }
    if ( node->className == GeometryClassId::SURFACE_MESH ) {
        // Note that empty meshes are being removed, this case will usually not show on Galerkin
        printf("Mesh (%d)\n", *elementCount);
        (*elementCount)++;
    } else if ( node->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = (const Compound *)node;
        printf("Compound %d (%d)\n", compound->id, *elementCount);
        (*elementCount)++;
        for ( int i = 0;
              compound->compoundData->children != nullptr && i < compound->compoundData->children->size();
              i++ ) {
            printClusterHierarchy(compound->compoundData->children->get(i), level + 1, elementCount);
        }

    } else if ( node->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = (const PatchSet *)node;
        if ( patchSet->getPatchList() == nullptr ) {
            printf("empty PatchSet (%d)\n", *elementCount);
        } else {
            printf("PatchSet %d with %ld patches (%d)\n", patchSet->id, patchSet->getPatchList()->size(), *elementCount);
        }
        (*elementCount)++;
    }
}

void
Scene::printVoxelGrid() const {
    printf("= voxelGrid ================================================================\n");
    voxelGrid->print();
}

void
Scene::print() const {
    printGeometries();
    printClusteredGeometries();
    printPatches();
    int elementCount = 0;
    printClusterHierarchy(clusteredRootGeometry, 0, &elementCount);
    printVoxelGrid();
    printf("*** Total number of geometry elements on cluster hierarchy: %d\n", elementCount);
}
