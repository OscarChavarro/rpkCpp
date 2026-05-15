#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "scene/Scene.h"

const char *Scene::compoundType = "Compound";
const char *Scene::meshSurfaceType = "MeshSurface";
const char *Scene::patchSetType = "PatchSet";
const char *Scene::unknownType = "<unknown>";

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
    clusteredGeometryList = new ArrayList<Geometry *>();
}

Scene::~Scene() {
    if ( lightSourcePatchList != NULL ) {
        delete lightSourcePatchList;
        lightSourcePatchList = NULL;
    }
    if ( clusteredGeometryList != NULL ) {
        delete clusteredGeometryList;
        clusteredGeometryList = NULL;
    }
    if ( clusteredRootGeometry != NULL ) {
        // This is deleted on Cluster::deleteCachedGeometries()
        clusteredRootGeometry = NULL;
    }
    if ( patchList != NULL ) {
        delete patchList;
        patchList = NULL;
    }
    if ( voxelGrid != NULL ) {
        delete voxelGrid;
        voxelGrid = NULL;
    }
    if ( background != NULL ) {
        delete background;
        background = NULL;
    }
    if ( camera != NULL ) {
        delete camera;
        camera = NULL;
    }
}

const char *
Scene::printGeometryType(GeometryClassId id) {
    const char *response = unknownType;
    if ( id == SURFACE_MESH ) {
        response = meshSurfaceType;
    } else if ( id == COMPOUND ) {
        response = compoundType;
    } else if ( id == PATCH_SET ) {
        response = patchSetType;
    }
    return response;
}

void
Scene::printPatchSet(const PatchSet *patchSet) {
    if ( patchSet->getPatchList() != NULL ) {
        System::out.printf("  - Patches: %ld\n", patchSet->getPatchList()->size());
    } else {
        System::out.printf("  - Patches: null list!\n");
    }
}

void
Scene::printCompound(const Compound *compound) {
    if ( compound->children != NULL ) {
        System::out.printf("    . Outer children: %ld\n", compound->children->size());
        for ( int i = 0; i < compound->children->size(); i++ ) {
            const Geometry *child = compound->children->get(i);
            System::out.printf("    . Child [%d] / [%s]\n", i, printGeometryType(child->className));
            if ( child->className == SURFACE_MESH ) {
                printSurfaceMesh(((const MeshSurface *)(child)), 6);
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

    System::out.printf("%s  - Object name: %s\n", spaces, mesh->objectName);
    System::out.printf("%s  - Type: SurfaceMesh\n", spaces);
    System::out.printf("%s    . Id: %d\n", spaces, mesh->id);
    System::out.printf("%s    . Inner id: %d\n", spaces, mesh->meshId);
    System::out.printf("%s    . Vertices: %ld, positions: %ld, normals: %ld, faces: %ld\n",
       spaces, mesh->vertices->size(), mesh->positions->size(), mesh->normals->size(), mesh->faces->size());

    delete[] spaces;
}

void
Scene::printGeometries() const {
    System::out.printf("= geometryList ================================================================\n");
    System::out.printf("Geometries on list: %ld\n", geometryList->size());
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        System::out.printf("  - Index: [%d of %ld] / [%s]\n", i + 1, geometryList->size(), printGeometryType(geometry->className));
        System::out.printf("    . Id: %d\n", geometry->id);
        System::out.printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");

        if ( geometry->className == SURFACE_MESH ) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            printSurfaceMesh(((MeshSurface *)(geometry)), 0);
        } else if ( geometry->className == COMPOUND ) {
            printCompound(((Compound *)(geometry)));
        } else if ( geometry->className == PATCH_SET ) {
            printPatchSet(((PatchSet *)(geometry)));
        }
    }
}

void
Scene::printClusteredGeometries() const {
    System::out.printf("= clusteredGeometryList ================================================================\n");
    System::out.printf("Geometry clusters on list: %ld\n", clusteredGeometryList->size());
    for ( int i = 0; i < clusteredGeometryList->size(); i++ ) {
        Geometry *geometry = clusteredGeometryList->get(i);
        System::out.printf("  - Index: [%d of %ld] / [%s]\n", i + 1, clusteredGeometryList->size(), printGeometryType(geometry->className));
        System::out.printf("    . Id: %d\n", geometry->id);
        System::out.printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");

        if ( geometry->className == SURFACE_MESH ) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            printSurfaceMesh(((MeshSurface *)(geometry)), 0);
        } else if ( geometry->className == COMPOUND ) {
            printCompound(((Compound *)(geometry)));
        } else if ( geometry->className == PATCH_SET ) {
            printPatchSet(((PatchSet *)(geometry)));
        }
    }
}

void
Scene::printPatches() const {
    System::out.printf("= patchList ================================================================\n");
    if ( patchList == NULL ) {
        System::out.printf("Patches on top level scene list: NULL\n");
        return;
    }
    System::out.printf("Patches on top level scene list: %ld\n", patchList->size());
    for ( int i = 0; i < patchList->size(); i++ ) {
        const Patch *patch = patchList->get(i);
        System::out.printf("  - patch[%d]: vertices: %d, area: %03f\n",
           i, patch->numberOfVertices, patch->area);
    }
}

void
Scene::printClusterHierarchy(const Geometry *node, int level, int *elementCount) {
    if ( level == 0 ) {
        System::out.printf("= clusteredRootGeometry ================================================================\n");
    }
    switch ( level ) {
        case 0:
            break;
        case 1:
            System::out.printf("* ");
            break;
        case 2:
            System::out.printf("  - ");
            break;
        case 3:
            System::out.printf("    . ");
            break;
        default:
            System::out.printf("   ");
            for ( int j = 0; j < level; j++ ) {
                System::out.printf(" ");
            }
            System::out.printf("[%d] ", level);
            break;
    }
    if ( node->className == SURFACE_MESH ) {
        // Note that empty meshes are being removed, this case will usually not show on Galerkin
        System::out.printf("Mesh (%d)\n", *elementCount);
        (*elementCount)++;
    } else if ( node->className == COMPOUND ) {
        const Compound *compound = ((const Compound *)(node));
        System::out.printf("Compound %d (%d)\n", compound->id, *elementCount);
        (*elementCount)++;
        for ( int i = 0;
              compound->children != NULL && i < compound->children->size();
              i++ ) {
            printClusterHierarchy(compound->children->get(i), level + 1, elementCount);
        }

    } else if ( node->className == PATCH_SET ) {
        const PatchSet *patchSet = ((const PatchSet *)(node));
        if ( patchSet->getPatchList() == NULL ) {
            System::out.printf("empty PatchSet (%d)\n", *elementCount);
        } else {
            System::out.printf("PatchSet %d with %ld patches (%d)\n", patchSet->id, patchSet->getPatchList()->size(), *elementCount);
        }
        (*elementCount)++;
    }
}

void
Scene::printVoxelGrid() const {
    System::out.printf("= voxelGrid ================================================================\n");
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
    System::out.printf("*** Total number of geometry elements on cluster hierarchy: %d\n", elementCount);
}
