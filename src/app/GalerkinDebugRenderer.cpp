#include <GL/gl.h>

#include "java/util/ArrayList.txx"

#include "GALERKIN/basisgalerkin.h"

#include "app/GalerkinDebugRenderer.h"

void
GalerkinDebugRenderer::recursiveDrawElement(const GalerkinElement *element, GalerkinElementRenderMode mode, const RenderOptions *renderOptions) {
    if ( element->regularSubElements == nullptr ) {
    } else {
        for ( int i = 0; i < 4; i++ ) {
            recursiveDrawElement(static_cast<GalerkinElement *>(element->regularSubElements[i]), mode, renderOptions);
        }
    }
}

void
GalerkinDebugRenderer::renderGalerkinElementHierarchy(
    const Scene *scene,
    const RenderOptions *renderOptions)
{
    glColor3d(1.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(1.0, 0.0, 0.0);
        glVertex3d(1.0, 1.0, 0.0);
        glVertex3d(0.0, 1.0, 0.0);
    glEnd();

    const GalerkinElement *root = static_cast<GalerkinElement *>(scene->clusteredRootGeometry->radianceData);

    recursiveDrawElement(root, GalerkinElementRenderMode::GOURAUD, renderOptions);
}
