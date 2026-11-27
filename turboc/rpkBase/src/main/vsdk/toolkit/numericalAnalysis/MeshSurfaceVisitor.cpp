#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/numericalAnalysis/MeshSurfaceVisitor.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"

/**
Initializes MeshSurface/Patch links with safe defaults without running numerical analysis.
*/
void
MeshSurfaceVisitor::initializeFacesDefaults(MeshSurface *mesh) {
    if ( mesh == NULL ) {
        return;
    }
    for ( int i = 0; mesh->faces != NULL && i < mesh->faces->size(); i++ ) {
        Patch *face = mesh->faces->get(i);
        if ( face == NULL ) {
            continue;
        }
        face->material = mesh->material;
    }
}

/**
Fills in the MeshSurface back pointer of the face belonging to the given surface
*/
void
MeshSurfaceVisitor::surfaceConnectFace(MeshSurface *mesh, Patch *face) {
    face->material = mesh->material;
    // Face display colors start black in the scene's FaceColorTable. The color
    // defaults that used to be derived here never applied: MeshSurface::colorFlags
    // is reset to NO_COLORS before the faces are connected.
}

/**
Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
*/
void
MeshSurfaceVisitor::fillFacesBackPointers(MeshSurface *mesh) {
    initializeFacesDefaults(mesh);
    for ( int i = 0; mesh->faces != NULL && i < mesh->faces->size(); i++ ) {
        MeshSurfaceVisitor::surfaceConnectFace(mesh, mesh->faces->get(i));
    }
}
