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
    const char *response = unknownType;
    if ( id == GeometryClassId::SURFACE_MESH ) {
        response = meshSurfaceType;
    } else if ( id == GeometryClassId::COMPOUND ) {
        response = compoundType;
    } else if ( id == GeometryClassId::PATCH_SET ) {
        response = patchSetType;
    }
    return response;
}

void
Scene::printPatchSet(const PatchSet *patchSet) {
    if ( patchSet->getPatchList() != nullptr ) {
        java::System::out.printf("  - Patches: %ld\n", patchSet->getPatchList()->size());
    } else {
        java::System::out.printf("  - Patches: null list!\n");
    }
}

void
Scene::printCompound(const Compound *compound) {
    if ( compound->children != nullptr ) {
        java::System::out.printf("    . Outer children: %ld\n", compound->children->size());
        for ( int i = 0; i < compound->children->size(); i++ ) {
            const Geometry *child = compound->children->get(i);
            java::System::out.printf("    . Child [%d] / [%s]\n", i, printGeometryType(child->className));
            if ( child->className == GeometryClassId::SURFACE_MESH ) {
                printSurfaceMesh(static_cast<const MeshSurface *>(child), 6);
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

    java::System::out.printf("%s  - Object name: %s\n", spaces, mesh->objectName);
    java::System::out.printf("%s  - Type: SurfaceMesh\n", spaces);
    java::System::out.printf("%s    . Id: %d\n", spaces, mesh->id);
    java::System::out.printf("%s    . Inner id: %d\n", spaces, mesh->meshId);
    java::System::out.printf("%s    . Vertices: %ld, positions: %ld, normals: %ld, faces: %ld\n",
       spaces, mesh->vertices->size(), mesh->positions->size(), mesh->normals->size(), mesh->faces->size());

    delete[] spaces;
}

void
Scene::printGeometries() const {
    java::System::out.printf("= geometryList ================================================================\n");
    java::System::out.printf("Geometries on list: %ld\n", geometryList->size());
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        java::System::out.printf("  - Index: [%d of %ld] / [%s]\n", i + 1, geometryList->size(), printGeometryType(geometry->className));
        java::System::out.printf("    . Id: %d\n", geometry->id);
        java::System::out.printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");

        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            printSurfaceMesh(static_cast<MeshSurface *>(geometry), 0);
        } else if ( geometry->className == GeometryClassId::COMPOUND ) {
            printCompound(static_cast<Compound *>(geometry));
        } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
            printPatchSet(static_cast<PatchSet *>(geometry));
        }
    }
}

void
Scene::printClusteredGeometries() const {
    java::System::out.printf("= clusteredGeometryList ================================================================\n");
    java::System::out.printf("Geometry clusters on list: %ld\n", clusteredGeometryList->size());
    for ( int i = 0; i < clusteredGeometryList->size(); i++ ) {
        Geometry *geometry = clusteredGeometryList->get(i);
        java::System::out.printf("  - Index: [%d of %ld] / [%s]\n", i + 1, clusteredGeometryList->size(), printGeometryType(geometry->className));
        java::System::out.printf("    . Id: %d\n", geometry->id);
        java::System::out.printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");

        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            printSurfaceMesh(static_cast<MeshSurface *>(geometry), 0);
        } else if ( geometry->className == GeometryClassId::COMPOUND ) {
            printCompound(static_cast<Compound *>(geometry));
        } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
            printPatchSet(static_cast<PatchSet *>(geometry));
        }
    }
}

void
Scene::printPatches() const {
    java::System::out.printf("= patchList ================================================================\n");
    if ( patchList == nullptr ) {
        java::System::out.printf("Patches on top level scene list: NULL\n");
        return;
    }
    java::System::out.printf("Patches on top level scene list: %ld\n", patchList->size());
    for ( int i = 0; i < patchList->size(); i++ ) {
        const Patch *patch = patchList->get(i);
        java::System::out.printf("  - patch[%d]: vertices: %d, area: %03f\n",
           i, patch->getNumberOfVertices(), patch->getArea());
    }
}

void
Scene::printClusterHierarchy(const Geometry *node, int level, int *elementCount) {
    if ( level == 0 ) {
        java::System::out.printf("= clusteredRootGeometry ================================================================\n");
    }
    switch ( level ) {
        case 0:
            break;
        case 1:
            java::System::out.printf("* ");
            break;
        case 2:
            java::System::out.printf("  - ");
            break;
        case 3:
            java::System::out.printf("    . ");
            break;
        default:
            java::System::out.printf("   ");
            for ( int j = 0; j < level; j++ ) {
                java::System::out.printf(" ");
            }
            java::System::out.printf("[%d] ", level);
            break;
    }
    if ( node->className == GeometryClassId::SURFACE_MESH ) {
        // Note that empty meshes are being removed, this case will usually not show on Galerkin
        java::System::out.printf("Mesh (%d)\n", *elementCount);
        (*elementCount)++;
    } else if ( node->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(node);
        java::System::out.printf("Compound %d (%d)\n", compound->id, *elementCount);
        (*elementCount)++;
        for ( int i = 0;
              compound->children != nullptr && i < compound->children->size();
              i++ ) {
            printClusterHierarchy(compound->children->get(i), level + 1, elementCount);
        }

    } else if ( node->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(node);
        if ( patchSet->getPatchList() == nullptr ) {
            java::System::out.printf("empty PatchSet (%d)\n", *elementCount);
        } else {
            java::System::out.printf("PatchSet %d with %ld patches (%d)\n", patchSet->id, patchSet->getPatchList()->size(), *elementCount);
        }
        (*elementCount)++;
    }
}

void
Scene::printVoxelGrid() const {
    java::System::out.printf("= voxelGrid ================================================================\n");
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
    java::System::out.printf("*** Total number of geometry elements on cluster hierarchy: %d\n", elementCount);
}
