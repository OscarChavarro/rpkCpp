#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugPatchHierarchy.h"

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/java/lang/Math.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/render/opengl/GalerkinOpenGLRenderer.h"
#include "vsdk/toolkit/render/opengl/Opengl.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

float
GlutDebugPatchHierarchy::clamp01(float value) {
    if ( value < 0.0F ) {
        return 0.0F;
    }
    if ( value > 1.0F ) {
        return 1.0F;
    }
    return value;
}

void
GlutDebugPatchHierarchy::addPatchIfNotPresent(
    java::ArrayList<const Patch *> *patches,
    const Patch *patch)
{
    if ( patches == nullptr || patch == nullptr ) {
        return;
    }

    for ( int i = 0; i < patches->size(); i++ ) {
        if ( patches->get(i) == patch ) {
            return;
        }
    }
    patches->add(patch);
}

float
GlutDebugPatchHierarchy::toneMappedGrayAndDarkened(float value01) {
    float adjusted = GlutDebugPatchHierarchy::clamp01(value01);
    adjusted = static_cast<float>(java::Math::pow(adjusted, GlutDebugPatchHierarchy::GRAY_CONTRAST_GAMMA));
    adjusted *= GlutDebugPatchHierarchy::GRAY_DARKEN_FACTOR;
    return GlutDebugPatchHierarchy::clamp01(adjusted);
}

int
GlutDebugPatchHierarchy::clampLevel(int level, int maxLevel) {
    if ( level < 0 ) {
        return 0;
    }
    return level > maxLevel ? maxLevel : level;
}

const GalerkinElement *
GlutDebugPatchHierarchy::selectedPatchRoot(const Scene *scene, int patchIndex) {
    if ( scene == nullptr || scene->patchList == nullptr ) {
        return nullptr;
    }
    if ( patchIndex < 0 || patchIndex >= scene->patchList->size() ) {
        return nullptr;
    }

    const Patch *patch = scene->patchList->get(patchIndex);
    if ( patch == nullptr || patch->getRadianceData() == nullptr ) {
        return nullptr;
    }
    if ( patch->getRadianceData()->className != ElementTypes::ELEMENT_GALERKIN ) {
        return nullptr;
    }

    return static_cast<const GalerkinElement *>(patch->getRadianceData());
}

int
GlutDebugPatchHierarchy::maxLevelFromElement(const GalerkinElement *element) {
    if ( element == nullptr || element->regularSubElements == nullptr ) {
        return 0;
    }

    int maxDepth = 0;
    for ( int i = 0; i < 4; i++ ) {
        const GalerkinElement *child = static_cast<const GalerkinElement *>(element->regularSubElements[i]);
        if ( child == nullptr ) {
            continue;
        }
        const int childDepth = 1 + maxLevelFromElement(child);
        if ( childDepth > maxDepth ) {
            maxDepth = childDepth;
        }
    }
    return maxDepth;
}

void
GlutDebugPatchHierarchy::renderElementGray(
    const GalerkinElement *element,
    const RendererConfiguration *renderOptions)
{
    if ( element == nullptr || renderOptions == nullptr ) {
        return;
    }
    const ToneMappingContext *toneMapOptions =
        element->galerkinState == nullptr ? nullptr : element->galerkinState->toneMapOptions;
    if ( toneMapOptions == nullptr ) {
        return;
    }
    if ( element->isCluster() ) {
        return;
    }

    Vector3D vertices[4];
    const int numberOfVertices = element->vertices(vertices);
    if ( numberOfVertices < 3 ) {
        return;
    }

    float grayValue = OUTLINE_MIN_GRAY;

    if ( renderOptions->drawSurfaces ) {
        ColorRgbMutable radianceSample{};
        if ( element->radiance != nullptr ) {
            radianceSample = element->radiance[0];
        }

        if ( element->galerkinState != nullptr
             && element->galerkinState->useAmbientRadiance
             && element->patch != nullptr
             && element->patch->getRadianceData() != nullptr ) {
            ColorRgbMutable ambient;
            ambient.scalarProduct(
                element->patch->getRadianceData()->Rd,
                element->galerkinState->ambientRadiance);
            radianceSample.add(radianceSample, ambient);
        }

        ColorRgbMutable rgbColor{};
        ToneMap::radianceToRgb(radianceSample, &rgbColor, *toneMapOptions);
        grayValue = toneMappedGrayAndDarkened(Cie::spectrumLuminance(rgbColor.getR(), rgbColor.getG(), rgbColor.getB()));
        glColor3f(grayValue, grayValue, grayValue);
        Opengl::openGlRenderPolygonFlat(numberOfVertices, vertices);
    }

    float outlineGray = grayValue;
    if ( renderOptions->drawSurfaces ) {
        outlineGray *= OUTLINE_FROM_SURFACE_FACTOR;
    } else {
        outlineGray = toneMappedGrayAndDarkened(Cie::spectrumLuminance(
            renderOptions->outlineColor.getR(),
            renderOptions->outlineColor.getG(),
            renderOptions->outlineColor.getB()));
    }
    outlineGray = clamp01(outlineGray);
    if ( outlineGray < OUTLINE_MIN_GRAY ) {
        outlineGray = OUTLINE_MIN_GRAY;
    }

    glColor3f(outlineGray, outlineGray, outlineGray);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const int nextIndex = (i + 1) % numberOfVertices;
        Opengl::openGlRenderLine(&vertices[i], &vertices[nextIndex]);
    }
}

void
GlutDebugPatchHierarchy::renderNonSelectedPatchesGray(
    const Scene *scene,
    const RendererConfiguration *renderOptions,
    int primaryPatchIndex,
    int secondaryPatchIndex,
    const java::ArrayList<Interaction *> *interactionsToRender)
{
    if ( scene == nullptr || renderOptions == nullptr || scene->patchList == nullptr ) {
        return;
    }

    const Patch *primaryPatch = nullptr;
    if ( primaryPatchIndex >= 0 && primaryPatchIndex < scene->patchList->size() ) {
        primaryPatch = scene->patchList->get(primaryPatchIndex);
    }

    for ( int patchIndex = 0; patchIndex < scene->patchList->size(); patchIndex++ ) {
        if ( patchIndex == primaryPatchIndex || patchIndex == secondaryPatchIndex ) {
            continue;
        }

        const Patch *patch = scene->patchList->get(patchIndex);
        if ( patch == nullptr || patch->getRadianceData() == nullptr ) {
            continue;
        }
        if ( patch->getRadianceData()->className != ElementTypes::ELEMENT_GALERKIN ) {
            continue;
        }

        if ( secondaryPatchIndex < 0 && interactionsToRender != nullptr ) {
            bool isInteractingWithPrimary = false;
            for ( int i = 0; i < interactionsToRender->size(); i++ ) {
                const Interaction *interaction = interactionsToRender->get(i);
                if ( interaction == nullptr ) {
                    continue;
                }

                const Patch *sourcePatch =
                    interaction->sourceElement == nullptr ? nullptr : interaction->sourceElement->patch;
                const Patch *receiverPatch =
                    interaction->receiverElement == nullptr ? nullptr : interaction->receiverElement->patch;

                const bool isPrimaryInteraction =
                    sourcePatch == primaryPatch || receiverPatch == primaryPatch;
                const bool isCurrentPatchInteraction =
                    sourcePatch == patch || receiverPatch == patch;

                if ( isPrimaryInteraction && isCurrentPatchInteraction ) {
                    isInteractingWithPrimary = true;
                    break;
                }
            }
            if ( isInteractingWithPrimary ) {
                continue;
            }
        }

        const GalerkinElement *element = dynamic_cast<const GalerkinElement *>(patch->getRadianceData());
        renderElementGray(element, renderOptions);
    }
}

void
GlutDebugPatchHierarchy::renderElementAtLevel(
    const GalerkinElement *element,
    int hierarchyLevel,
    const RendererConfiguration *renderOptions)
{
    if ( element == nullptr || renderOptions == nullptr ) {
        return;
    }

    if ( hierarchyLevel <= 0 || element->regularSubElements == nullptr ) {
        GalerkinOpenGLRenderer::drawElement(element, GalerkinElement::renderMode(renderOptions), renderOptions);
        return;
    }

    for ( int i = 0; i < 4; i++ ) {
        const GalerkinElement *child = static_cast<const GalerkinElement *>(element->regularSubElements[i]);
        if ( child != nullptr ) {
            renderElementAtLevel(child, hierarchyLevel - 1, renderOptions);
        }
    }
}

void
GlutDebugPatchHierarchy::drawCenterMark(
    const Vector3D &center,
    float radius,
    int sides,
    const Vector3D &axisU,
    const Vector3D &axisV,
    const RendererConfiguration *renderOptions)
{
    if ( renderOptions == nullptr || sides < 3 || radius < Numeric::EPSILON ) {
        return;
    }

    const ColorRgbMutable yellow(1.0F, 1.0F, 0.0F);
    Opengl::openGlRenderSetColor(&yellow, renderOptions);

    constexpr float TWO_PI = 6.28318530717958647692F;
    Vector3D firstVertex;
    Vector3D previousVertex;
    for ( int i = 0; i < sides; i++ ) {
        const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(sides);
        const float u = radius * java::Math::cos(angle);
        const float v = radius * java::Math::sin(angle);

        Vector3D currentVertex;
        currentVertex.combine3(center, u, axisU, v, axisV);

        if ( i == 0 ) {
            firstVertex = currentVertex;
        } else {
            Opengl::openGlRenderLine(&previousVertex, &currentVertex);
        }
        previousVertex = currentVertex;
    }
    Opengl::openGlRenderLine(&previousVertex, &firstVertex);
}

void
GlutDebugPatchHierarchy::drawSelectedPatchCenterMarker(
    const GalerkinElement *topLevelElement,
    const RendererConfiguration *renderOptions)
{
    if ( topLevelElement == nullptr || renderOptions == nullptr ) {
        return;
    }

    Vector3D vertices[8];
    const int numberOfVertices = topLevelElement->vertices(vertices);
    if ( numberOfVertices < 3 ) {
        return;
    }

    Vector3D axisU;
    axisU.subtraction(vertices[1], vertices[0]);
    if ( axisU.norm2() < Numeric::EPSILON ) {
        axisU.subtraction(vertices[2], vertices[0]);
    }
    if ( axisU.norm2() < Numeric::EPSILON ) {
        axisU.set(1.0F, 0.0F, 0.0F);
    } else {
        axisU.normalize(Numeric::EPSILON);
    }

    Vector3D normal;
    if ( topLevelElement->patch != nullptr ) {
        normal.copy(topLevelElement->patch->getNormal());
    } else {
        Vector3D edgeA;
        Vector3D edgeB;
        edgeA.subtraction(vertices[1], vertices[0]);
        edgeB.subtraction(vertices[2], vertices[0]);
        normal.crossProduct(edgeA, edgeB);
    }
    if ( normal.norm2() < Numeric::EPSILON ) {
        normal.set(0.0F, 0.0F, 1.0F);
    } else {
        normal.normalize(Numeric::EPSILON);
    }

    Vector3D axisV;
    axisV.crossProduct(normal, axisU);
    if ( axisV.norm2() < Numeric::EPSILON ) {
        axisV.set(0.0F, 1.0F, 0.0F);
    } else {
        axisV.normalize(Numeric::EPSILON);
    }

    float averageEdgeSize = 0.0F;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const int nextIndex = (i + 1) % numberOfVertices;
        averageEdgeSize += vertices[i].distance(vertices[nextIndex]);
    }
    averageEdgeSize /= static_cast<float>(numberOfVertices);
    if ( averageEdgeSize < Numeric::EPSILON ) {
        averageEdgeSize = 0.1F;
    }

    const float radius = 0.08F * averageEdgeSize;
    const Vector3D center = topLevelElement->midPoint();
    drawCenterMark(center, radius, 8, axisU, axisV, renderOptions);
}

void
GlutDebugPatchHierarchy::drawGradientLine(
    const Vector3D &start,
    const Vector3D &end)
{
    glBegin(GL_LINES);
    glColor3f(0.5F, 0.5F, 0.5F);
    glVertex3f(start.x, start.y, start.z);
    glColor3f(1.0F, 1.0F, 1.0F);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
}

void
GlutDebugPatchHierarchy::drawInteractions(
    const java::ArrayList<Interaction *> *interactionsToRender)
{
    if ( interactionsToRender == nullptr ) {
        return;
    }

    GLint previousShadeModel = GL_FLAT;
    glGetIntegerv(GL_SHADE_MODEL, &previousShadeModel);
    glShadeModel(GL_SMOOTH);

    GLfloat previousLineWidth = 1.0F;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glLineWidth(2.0F);

    for ( int i = 0; i < interactionsToRender->size(); i++ ) {
        const Interaction *interaction = interactionsToRender->get(i);
        if ( interaction == nullptr
             || interaction->sourceElement == nullptr
             || interaction->receiverElement == nullptr ) {
            continue;
        }

        GlutDebugPatchHierarchy::drawGradientLine(
            interaction->sourceElement->midPoint(),
            interaction->receiverElement->midPoint());
    }

    glLineWidth(previousLineWidth);
    glShadeModel(previousShadeModel);
}

void
GlutDebugPatchHierarchy::renderInteractionBetweenSelected(
    const Scene *scene,
    int primaryPatchIndex,
    int secondaryPatchIndex,
    const java::ArrayList<Interaction *> *interactionsToRender)
{
    (void)secondaryPatchIndex;
    if ( scene == nullptr ) {
        return;
    }
    if ( primaryPatchIndex < 0 ) {
        return;
    }
    drawInteractions(interactionsToRender);
}

void
GlutDebugPatchHierarchy::renderInteractingPatchesAtLevelIfNoSecondary(
    const Scene *scene,
    const RendererConfiguration *renderOptions,
    int primaryPatchIndex,
    int secondaryPatchIndex,
    int hierarchyLevel,
    const java::ArrayList<Interaction *> *interactionsToRender)
{
    if ( scene == nullptr || renderOptions == nullptr || interactionsToRender == nullptr ) {
        return;
    }
    if ( scene->patchList == nullptr || primaryPatchIndex < 0 || primaryPatchIndex >= scene->patchList->size() ) {
        return;
    }
    if ( secondaryPatchIndex >= 0 ) {
        return;
    }

    const Patch *primaryPatch = scene->patchList->get(primaryPatchIndex);
    if ( primaryPatch == nullptr ) {
        return;
    }

    java::ArrayList<const Patch *> interactingPatches;
    for ( int i = 0; i < interactionsToRender->size(); i++ ) {
        const Interaction *interaction = interactionsToRender->get(i);
        if ( interaction == nullptr ) {
            continue;
        }

        const Patch *sourcePatch =
            interaction->sourceElement == nullptr ? nullptr : interaction->sourceElement->patch;
        const Patch *receiverPatch =
            interaction->receiverElement == nullptr ? nullptr : interaction->receiverElement->patch;

        if ( sourcePatch != primaryPatch ) {
            GlutDebugPatchHierarchy::addPatchIfNotPresent(&interactingPatches, sourcePatch);
        }
        if ( receiverPatch != primaryPatch ) {
            GlutDebugPatchHierarchy::addPatchIfNotPresent(&interactingPatches, receiverPatch);
        }
    }

    if ( interactingPatches.size() <= 0 ) {
        return;
    }

    RendererConfiguration interactingRenderOptions = *renderOptions;
    interactingRenderOptions.drawSurfaces = true;
    interactingRenderOptions.drawOutlines = true;
    interactingRenderOptions.outlineColor = ColorRgbMutable(1.0F, 1.0F, 0.0F);

    GLint previousDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
    GLfloat previousLineWidth = 1.0F;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(2.0F);

    for ( int i = 0; i < interactingPatches.size(); i++ ) {
        const GalerkinElement *topLevelElement = GalerkinElement::fromPatch(interactingPatches.get(i));
        if ( topLevelElement == nullptr || topLevelElement->isCluster() ) {
            continue;
        }

        const int maxLevel = maxLevelFromElement(topLevelElement);
        const int clampedLevel = clampLevel(hierarchyLevel, maxLevel);
        renderElementAtLevel(topLevelElement, clampedLevel, &interactingRenderOptions);
    }

    glLineWidth(previousLineWidth);
    glDepthFunc(previousDepthFunc);
}

void
GlutDebugPatchHierarchy::drawSecondarySelectedPatchMarker(
    const GalerkinElement *topLevelElement,
    const RendererConfiguration *renderOptions,
    int hierarchyLevel)
{
    if ( topLevelElement == nullptr || renderOptions == nullptr || topLevelElement->isCluster() ) {
        return;
    }

    const int maxLevel = maxLevelFromElement(topLevelElement);
    const int secondaryHierarchyLevel = clampLevel(hierarchyLevel, maxLevel);

    RendererConfiguration secondaryRenderOptions = *renderOptions;
    secondaryRenderOptions.drawSurfaces = true;
    secondaryRenderOptions.drawOutlines = true;
    secondaryRenderOptions.outlineColor = ColorRgbMutable(1.0F, 1.0F, 0.0F);

    GLint previousDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
    GLfloat previousLineWidth = 1.0F;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(2.0F);

    renderElementAtLevel(topLevelElement, secondaryHierarchyLevel, &secondaryRenderOptions);

    glLineWidth(previousLineWidth);
    glDepthFunc(previousDepthFunc);
}

int
GlutDebugPatchHierarchy::maxLevelForSelectedPatch(const Scene *scene, int patchIndex) {
    const GalerkinElement *topLevelElement = selectedPatchRoot(scene, patchIndex);
    return maxLevelFromElement(topLevelElement);
}

int
GlutDebugPatchHierarchy::maxLevelAcrossScene(const Scene *scene) {
    if ( scene == nullptr || scene->patchList == nullptr ) {
        return 0;
    }

    int maxLevel = 0;
    for ( int patchIndex = 0; patchIndex < scene->patchList->size(); patchIndex++ ) {
        const GalerkinElement *topLevelElement = selectedPatchRoot(scene, patchIndex);
        const int patchMaxLevel = maxLevelFromElement(topLevelElement);
        if ( patchMaxLevel > maxLevel ) {
            maxLevel = patchMaxLevel;
        }
    }
    return maxLevel;
}

void
GlutDebugPatchHierarchy::renderSelectedPatchAtLevel(
    const Scene *scene,
    const RendererConfiguration *renderOptions,
    int primaryPatchIndex,
    int secondaryPatchIndex,
    int hierarchyLevel,
    const java::ArrayList<Interaction *> *interactionsToRender)
{
    if ( renderOptions == nullptr ) {
        return;
    }

    renderNonSelectedPatchesGray(
        scene,
        renderOptions,
        primaryPatchIndex,
        secondaryPatchIndex,
        interactionsToRender);

    const GalerkinElement *primaryTopLevelElement = selectedPatchRoot(scene, primaryPatchIndex);
    if ( primaryTopLevelElement != nullptr ) {
        const int maxLevel = maxLevelFromElement(primaryTopLevelElement);
        const int clampedLevel = clampLevel(hierarchyLevel, maxLevel);

        RendererConfiguration selectedRenderOptions = *renderOptions;
        selectedRenderOptions.drawSurfaces = true;
        selectedRenderOptions.drawOutlines = true;

        renderElementAtLevel(primaryTopLevelElement, clampedLevel, &selectedRenderOptions);
        drawSelectedPatchCenterMarker(primaryTopLevelElement, renderOptions);
    }
}

void
GlutDebugPatchHierarchy::renderSecondarySelectedPatchMarker(
    const Scene *scene,
    const RendererConfiguration *renderOptions,
    int secondaryPatchIndex,
    int hierarchyLevel)
{
    if ( scene == nullptr || renderOptions == nullptr ) {
        return;
    }

    const GalerkinElement *secondaryTopLevelElement = selectedPatchRoot(scene, secondaryPatchIndex);
    drawSecondarySelectedPatchMarker(secondaryTopLevelElement, renderOptions, hierarchyLevel);
}
