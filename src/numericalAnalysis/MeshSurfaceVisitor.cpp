#include "java/util/ArrayList.txx"

#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "numericalAnalysis/PatchVisitor.h"

/**
Initializes MeshSurface/Patch links with safe defaults without running numerical analysis.
*/
void
MeshSurfaceVisitor::initializeFacesDefaults(MeshSurface *mesh) {
    if ( mesh == nullptr ) {
        return;
    }
    for ( int i = 0; mesh->faces != nullptr && i < mesh->faces->size(); i++ ) {
        Patch *face = mesh->faces->get(i);
        if ( face == nullptr ) {
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
    int i;

    face->material = mesh->material;

    // Also fill in a nicer default color for the patch
    switch ( mesh->colorFlags ) {
        case MaterialColorFlags::FACE_COLORS:
            break;
        case MaterialColorFlags::VERTEX_COLORS:
            // Average color of the vertices
            face->color.set(0, 0, 0);
            for ( i = 0; i < face->numberOfVertices; i++ ) {
                face->color.r += face->vertex[i]->color.r;
                face->color.g += face->vertex[i]->color.g;
                face->color.b += face->vertex[i]->color.b;
            }
            face->color.r /= static_cast<float>(i);
            face->color.g /= static_cast<float>(i);
            face->color.b /= static_cast<float>(i);
            break;
        default: {
            ColorRgb rho;
            rho = PatchVisitor::averageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            rho.set(face->color.r, face->color.g, face->color.b);
        }
    }
}

/**
Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
*/
void
MeshSurfaceVisitor::fillFacesBackPointers(MeshSurface *mesh) {
    initializeFacesDefaults(mesh);
    for ( int i = 0; mesh->faces != nullptr && i < mesh->faces->size(); i++ ) {
        MeshSurfaceVisitor::surfaceConnectFace(mesh, mesh->faces->get(i));
    }
}
