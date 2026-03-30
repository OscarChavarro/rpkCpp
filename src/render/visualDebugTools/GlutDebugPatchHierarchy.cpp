#include "render/visualDebugTools/GlutDebugPatchHierarchy.h"

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "galerkin/GalerkinElement.h"
#include "java/util/ArrayList.txx"
#include "render/Opengl.h"
#include "scene/Scene.h"

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
    if ( patch == nullptr || patch->radianceData == nullptr ) {
        return nullptr;
    }
    if ( patch->radianceData->className != ElementTypes::ELEMENT_GALERKIN ) {
        return nullptr;
    }

    return static_cast<const GalerkinElement *>(patch->radianceData);
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
GlutDebugPatchHierarchy::renderElementAtLevel(
    const GalerkinElement *element,
    int hierarchyLevel,
    const RenderOptions *renderOptions)
{
    if ( element == nullptr || renderOptions == nullptr ) {
        return;
    }

    if ( hierarchyLevel <= 0 || element->regularSubElements == nullptr ) {
        element->render(renderOptions);
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
GlutDebugPatchHierarchy::drawSelectedPatchCenterMarker(
    const GalerkinElement *topLevelElement,
    const RenderOptions *renderOptions)
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
        axisU.set(1.0f, 0.0f, 0.0f);
    } else {
        axisU.normalize(Numeric::EPSILON);
    }

    Vector3D normal;
    if ( topLevelElement->patch != nullptr ) {
        normal.copy(topLevelElement->patch->normal);
    } else {
        Vector3D edgeA;
        Vector3D edgeB;
        edgeA.subtraction(vertices[1], vertices[0]);
        edgeB.subtraction(vertices[2], vertices[0]);
        normal.crossProduct(edgeA, edgeB);
    }
    if ( normal.norm2() < Numeric::EPSILON ) {
        normal.set(0.0f, 0.0f, 1.0f);
    } else {
        normal.normalize(Numeric::EPSILON);
    }

    Vector3D axisV;
    axisV.crossProduct(normal, axisU);
    if ( axisV.norm2() < Numeric::EPSILON ) {
        axisV.set(0.0f, 1.0f, 0.0f);
    } else {
        axisV.normalize(Numeric::EPSILON);
    }

    float averageEdgeSize = 0.0f;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const int nextIndex = (i + 1) % numberOfVertices;
        averageEdgeSize += vertices[i].distance(vertices[nextIndex]);
    }
    averageEdgeSize /= static_cast<float>(numberOfVertices);
    if ( averageEdgeSize < Numeric::EPSILON ) {
        averageEdgeSize = 0.1f;
    }

    const float halfSize = 0.08f * averageEdgeSize;
    Vector3D center = topLevelElement->midPoint();
    Vector3D normalOffset;
    normalOffset.scaledCopy(0.002f * averageEdgeSize, normal);

    Vector3D markerVertices[4];
    markerVertices[0].combine3(center, -halfSize, axisU, -halfSize, axisV);
    markerVertices[1].combine3(center,  halfSize, axisU, -halfSize, axisV);
    markerVertices[2].combine3(center,  halfSize, axisU,  halfSize, axisV);
    markerVertices[3].combine3(center, -halfSize, axisU,  halfSize, axisV);
    for ( auto &markerVertex : markerVertices ) {
        markerVertex.addition(markerVertex, normalOffset);
    }

    const ColorRgb yellow(1.0f, 1.0f, 0.0f);
    Opengl::openGlRenderSetColor(&yellow, renderOptions);
    Opengl::openGlRenderLine(&markerVertices[0], &markerVertices[1]);
    Opengl::openGlRenderLine(&markerVertices[1], &markerVertices[2]);
    Opengl::openGlRenderLine(&markerVertices[2], &markerVertices[3]);
    Opengl::openGlRenderLine(&markerVertices[3], &markerVertices[0]);
}

int
GlutDebugPatchHierarchy::maxLevelForSelectedPatch(const Scene *scene, int patchIndex) {
    const GalerkinElement *topLevelElement = selectedPatchRoot(scene, patchIndex);
    return maxLevelFromElement(topLevelElement);
}

void
GlutDebugPatchHierarchy::renderSelectedPatchAtLevel(
    const Scene *scene,
    const RenderOptions *renderOptions,
    int patchIndex,
    int hierarchyLevel)
{
    const GalerkinElement *topLevelElement = selectedPatchRoot(scene, patchIndex);
    if ( topLevelElement == nullptr || renderOptions == nullptr ) {
        return;
    }

    const int maxLevel = maxLevelFromElement(topLevelElement);
    const int clampedLevel = clampLevel(hierarchyLevel, maxLevel);

    RenderOptions localRenderOptions = *renderOptions;
    localRenderOptions.drawSurfaces = true;
    localRenderOptions.drawOutlines = true;

    renderElementAtLevel(topLevelElement, clampedLevel, &localRenderOptions);
    drawSelectedPatchCenterMarker(topLevelElement, renderOptions);
}
