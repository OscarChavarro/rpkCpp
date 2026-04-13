#include "render/opengl/GalerkinOpenGLRenderer.h"

#include "galerkin/GalerkinBasis.h"
#include "galerkin/GalerkinElement.h"
#include "java/util/ArrayList.txx"
#include "render/opengl/RenderOpenGL.h"
#include "render/opengl/Opengl.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "tonemap/ToneMap.h"

void
GalerkinOpenGLRenderer::galerkinRenderPatch(
    const Patch *patch,
    const Camera * /*camera*/,
    const RenderOptions *renderOptions)
{
    if ( patch == nullptr ) {
        return;
    }
    GalerkinOpenGLRenderer::renderElementHierarchy(GalerkinElement::fromPatch(patch), renderOptions);
}

void
GalerkinOpenGLRenderer::renderElementHierarchy(const GalerkinElement *element, const RenderOptions *renderOptions) {
    if ( element == nullptr || renderOptions == nullptr ) {
        return;
    }

    if ( element->regularSubElements == nullptr ) {
        GalerkinOpenGLRenderer::drawElement(element, GalerkinElement::renderMode(renderOptions), renderOptions);
    } else {
        for ( int i = 0; i < 4; i++ ) {
            const GalerkinElement *child = static_cast<const GalerkinElement *>(element->regularSubElements[i]);
            GalerkinOpenGLRenderer::renderElementHierarchy(child, renderOptions);
        }
    }
}

void
GalerkinOpenGLRenderer::drawElement(const GalerkinElement *element, int mode, const RenderOptions *renderOptions) {
    if ( element == nullptr || renderOptions == nullptr ) {
        return;
    }

    if ( element->isCluster() ) {
        if ( mode & GalerkinElementRenderMode::OUTLINE ) {
            RenderOpenGL::renderBoundingBox(element->geometry->getBoundingBox());
        }
        return;
    }

    Vector3D p[4];
    int numberOfVertices = element->vertices(p);

    // Draw surfaces
    if ( renderOptions->drawSurfaces ) {
        const ToneMappingContext *toneMapOptions =
            element->galerkinState == nullptr ? nullptr : element->galerkinState->toneMapOptions;
        if ( toneMapOptions == nullptr ) {
            return;
        }

        if ( mode & GalerkinElementRenderMode::FLAT ) {
            ColorRgb color{};
            ColorRgb rho = element->patch->radianceData->Rd;

            if ( element->galerkinState->useAmbientRadiance ) {
                ColorRgb radVis;
                radVis.scalarProduct(rho, element->galerkinState->ambientRadiance);
                radVis.add(radVis, element->radiance[0]);
                ToneMap::radianceToRgb(radVis, &color, *toneMapOptions);
            } else {
                ToneMap::radianceToRgb(element->radiance[0], &color, *toneMapOptions);
            }
            Opengl::openGlRenderSetColor(&color, renderOptions);
            Opengl::openGlRenderPolygonFlat(numberOfVertices, p);
        } else if ( mode & GalerkinElementRenderMode::GOURAUD ) {
            ColorRgb vertRadiosity[4];

            if ( numberOfVertices == 3 ) {
                vertRadiosity[0] = GalerkinBasis::radianceAtPoint(element, element->radiance, 0.0, 0.0);
                vertRadiosity[1] = GalerkinBasis::radianceAtPoint(element, element->radiance, 1.0, 0.0);
                vertRadiosity[2] = GalerkinBasis::radianceAtPoint(element, element->radiance, 0.0, 1.0);
            } else {
                vertRadiosity[0] = GalerkinBasis::radianceAtPoint(element, element->radiance, 0.0, 0.0);
                vertRadiosity[1] = GalerkinBasis::radianceAtPoint(element, element->radiance, 1.0, 0.0);
                vertRadiosity[2] = GalerkinBasis::radianceAtPoint(element, element->radiance, 1.0, 1.0);
                vertRadiosity[3] = GalerkinBasis::radianceAtPoint(element, element->radiance, 0.0, 1.0);
            }

            if ( element->galerkinState->useAmbientRadiance ) {
                ColorRgb reflectivity = element->patch->radianceData->Rd;
                ColorRgb ambient;

                ambient.scalarProduct(reflectivity, element->galerkinState->ambientRadiance);
                for ( int i = 0; i < numberOfVertices; i++ ) {
                    vertRadiosity[i].add(vertRadiosity[i], ambient);
                }
            }

            ColorRgb vertexColors[4];
            for ( int i = 0; i < numberOfVertices; i++ ) {
                ToneMap::radianceToRgb(vertRadiosity[i], &vertexColors[i], *toneMapOptions);
            }

            Opengl::openGlRenderPolygonGouraud(numberOfVertices, p, vertexColors, renderOptions);
        }
    }

    // Draw outlines
    if ( mode & GalerkinElementRenderMode::OUTLINE ) {
        Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
        if ( numberOfVertices == 3 ) {
            Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
            Opengl::openGlRenderLine(&p[0], &p[1]);
            Opengl::openGlRenderLine(&p[1], &p[2]);
            Opengl::openGlRenderLine(&p[2], &p[0]);
        } else {
            ColorRgb green = {0.0, 1.0, 0.0};

            Opengl::openGlRenderSetColor(&green, renderOptions);
            Opengl::openGlRenderLine(&p[0], &p[1]);
            Opengl::openGlRenderLine(&p[1], &p[2]);
            Opengl::openGlRenderLine(&p[2], &p[3]);
            Opengl::openGlRenderLine(&p[3], &p[0]);
        }
    }
}

void
GalerkinOpenGLRenderer::renderScene(
    const Scene *scene,
    const RenderOptions *renderOptions,
    const GlutDebugState *debugState)
{
    if ( scene == nullptr || renderOptions == nullptr ) {
        return;
    }

    if ( renderOptions->frustumCulling ) {
        Opengl::openGlRenderWorldOctree(scene, GalerkinOpenGLRenderer::galerkinRenderPatch, renderOptions);
        return;
    }

    if ( debugState == nullptr ) {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            GalerkinOpenGLRenderer::galerkinRenderPatch(scene->patchList->get(i), scene->camera, renderOptions);
        }
        return;
    }

    RenderOptions modifiedRenderOptions = *renderOptions;
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        if ( debugState->showSelectedPathOnly ) {
            if ( i == debugState->primarySelectedPatch ) {
                modifiedRenderOptions.drawOutlines = true;
                modifiedRenderOptions.outlineColor = ColorRgb(1.0f, 0.0f, 0.0f);
            } else {
                modifiedRenderOptions.drawOutlines = false;
            }
            GalerkinOpenGLRenderer::galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
        } else {
            modifiedRenderOptions.outlineColor = ColorRgb(0.4f, 0.1f, 0.1f);
            if ( i == debugState->primarySelectedPatch ) {
                modifiedRenderOptions.outlineColor = ColorRgb(0.0f, 0.0f, 1.0f);
            }
            GalerkinOpenGLRenderer::galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
        }
    }
}
