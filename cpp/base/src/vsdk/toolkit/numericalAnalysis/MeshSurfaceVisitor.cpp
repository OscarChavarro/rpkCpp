#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/numericalAnalysis/MeshSurfaceVisitor.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"

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
        face->setMaterial(mesh->material);
    }
}

/**
Fills in the MeshSurface back pointer of the face belonging to the given surface
*/
void
MeshSurfaceVisitor::surfaceConnectFace(MeshSurface *mesh, Patch *face) {
    int i;

    face->setMaterial(mesh->material);

    // Also fill in a nicer default color for the patch
    switch ( mesh->colorFlags ) {
        case MaterialColorFlags::FACE_COLORS:
            break;
        case MaterialColorFlags::VERTEX_COLORS: {
            // Average color of the vertices
            ColorRgb patchColor;
            patchColor.set(0, 0, 0);
            for ( i = 0; i < face->getNumberOfVertices(); i++ ) {
                patchColor.setR(patchColor.getR() + face->getVertices()[i]->color.getR());
                patchColor.setG(patchColor.getG() + face->getVertices()[i]->color.getG());
                patchColor.setB(patchColor.getB() + face->getVertices()[i]->color.getB());
            }
            patchColor.setR(patchColor.getR() / static_cast<double>(i));
            patchColor.setG(patchColor.getG() / static_cast<double>(i));
            patchColor.setB(patchColor.getB() / static_cast<double>(i));
            face->setColor(patchColor);
            break;
        }
        default: {
            ColorRgb rho;
            rho = PatchVisitor::averageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            rho.set(face->getColor().getR(), face->getColor().getG(), face->getColor().getB());
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
