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
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;
            for ( i = 0; i < face->getNumberOfVertices(); i++ ) {
                r += face->getVertices()[i]->color.getR();
                g += face->getVertices()[i]->color.getG();
                b += face->getVertices()[i]->color.getB();
            }
            const double invCount = 1.0 / static_cast<double>(i);
            const ColorRgbMutable patchColor(r * invCount, g * invCount, b * invCount);
            face->setColor(patchColor);
            break;
        }
        default: {
            ColorRgbMutable rho(0.0, 0.0, 0.0);
            rho = PatchVisitor::averageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            rho = ColorRgbMutable(face->getColor().getR(), face->getColor().getG(), face->getColor().getB());
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
