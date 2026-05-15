#include "java/util/ArrayList.txx"
#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "numericalAnalysis/PatchVisitor.h"

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
    int i;

    face->material = mesh->material;

    // Also fill in a nicer default color for the patch
    switch ( mesh->colorFlags ) {
        case FACE_COLORS:
            break;
        case VERTEX_COLORS:
            // Average color of the vertices
            {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            for ( i = 0; i < face->numberOfVertices; i++ ) {
                r += face->vertex[i]->color.getR();
                g += face->vertex[i]->color.getG();
                b += face->vertex[i]->color.getB();
            }
            const float inv = 1.0f / ((float)(i));
            face->color = ColorRgb(r * inv, g * inv, b * inv);
            }
            break;
        default: {
            ColorRgb rho;
            rho = PatchVisitor::averageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            face->color = rho;
        }
    }
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
