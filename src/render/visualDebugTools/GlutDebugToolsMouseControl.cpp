#include "render/visualDebugTools/GlutDebugToolsMouseControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <cstdlib>

#include "common/linealAlgebra/Matrix4x4.h"
#include "java/util/ArrayList.txx"
#include "material/RayHit.h"
#include "material/RayHitFlag.h"
#include "render/Render.h"
#include "render/visualDebugTools/GlutDebugPatchHierarchy.h"
#include "render/visualDebugTools/GlutDebugState.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "skin/Patch.h"

namespace {

static constexpr int DRAG_START_THRESHOLD_PIXELS = 3;
static constexpr float DRAG_ROTATION_DEGREES_PER_PIXEL = 0.25f;
static constexpr float DEGREES_TO_RADIANS = 3.14159265358979323846f / 180.0f;

static bool globalLeftButtonDown = false;
static bool globalDragging = false;
static bool globalPressWithShift = false;
static int globalPressX = 0;
static int globalPressY = 0;
static int globalLastX = 0;
static int globalLastY = 0;

struct PatchHitCandidate {
    int patchIndex;
    float distance;
    bool frontFacing;
};

bool
applyPatchSelection(
    int pickedPatchIndex,
    int *selectedPatch)
{
    if ( selectedPatch == nullptr ) {
        return false;
    }

    if ( pickedPatchIndex < 0 ) {
        if ( *selectedPatch == -1 ) {
            return false;
        }
        *selectedPatch = -1;
        return true;
    }

    if ( *selectedPatch == pickedPatchIndex ) {
        *selectedPatch = -1;
        return true;
    }

    *selectedPatch = pickedPatchIndex;
    return true;
}

void
syncCameraToViewport(const GlutDebugToolsModel &model) {
    if ( model.scene == nullptr || model.scene->camera == nullptr ) {
        return;
    }
    if ( model.width <= 0 || model.height <= 0 ) {
        return;
    }

    Camera *camera = model.scene->camera;
    if ( camera->xSize == model.width &&
         camera->ySize == model.height &&
         camera->pixelWidth > Numeric::EPSILON_FLOAT &&
         camera->pixelHeight > Numeric::EPSILON_FLOAT ) {
        return;
    }

    camera->set(
        &camera->eyePosition,
        &camera->lookPosition,
        &camera->upDirection,
        camera->fieldOfVision,
        model.width,
        model.height,
        &camera->background);
}

}

int
GlutDebugToolsMouseControl::clampCoord(int value, int maxExclusive) {
    if ( maxExclusive <= 0 ) {
        return 0;
    }
    if ( value < 0 ) {
        return 0;
    }
    if ( value >= maxExclusive ) {
        return maxExclusive - 1;
    }
    return value;
}

void
GlutDebugToolsMouseControl::clampSelectedHierarchyLevel(GlutDebugToolsModel &model) {
    const int maxHierarchyLevel = GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
        model.scene,
        GLOBAL_render_glutDebugState.primarySelectedPatch);

    if ( model.selectedHierarchyLevel < 0 ) {
        model.selectedHierarchyLevel = 0;
    }
    if ( model.selectedHierarchyLevel > maxHierarchyLevel ) {
        model.selectedHierarchyLevel = maxHierarchyLevel;
    }
}

Vector3D
GlutDebugToolsMouseControl::sceneRotationPivot(const Scene *scene) {
    if ( scene == nullptr ) {
        return Vector3D(0.0f, 0.0f, 0.0f);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        BoundingBox sceneBounds;
        Geometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3D(0.0f, 0.0f, 0.0f);
}

void
GlutDebugToolsMouseControl::viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    axisU->set(1.0f, 0.0f, 0.0f);
    axisV->set(0.0f, 1.0f, 0.0f);

    if ( scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    const Camera *camera = scene->camera;

    Vector3D cameraU;
    cameraU.copy(camera->X);
    Vector3D cameraV;
    cameraV.copy(camera->Y);

    if ( cameraU.norm2() < Numeric::EPSILON_FLOAT || cameraV.norm2() < Numeric::EPSILON_FLOAT ) {
        Vector3D viewDirection;
        viewDirection.subtraction(camera->lookPosition, camera->eyePosition);
        if ( viewDirection.norm2() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        viewDirection.normalize(Numeric::EPSILON_FLOAT);

        Vector3D upDirection;
        upDirection.copy(camera->upDirection);
        if ( upDirection.norm2() < Numeric::EPSILON_FLOAT ) {
            upDirection.set(0.0f, 0.0f, 1.0f);
        } else {
            upDirection.normalize(Numeric::EPSILON_FLOAT);
        }

        cameraU.crossProduct(viewDirection, upDirection);
        if ( cameraU.norm2() < Numeric::EPSILON_FLOAT ) {
            upDirection.set(0.0f, 1.0f, 0.0f);
            cameraU.crossProduct(viewDirection, upDirection);
        }
        if ( cameraU.norm2() < Numeric::EPSILON_FLOAT ) {
            return;
        }

        cameraU.normalize(Numeric::EPSILON_FLOAT);
        cameraV.crossProduct(viewDirection, cameraU);
    }

    if ( cameraU.norm2() < Numeric::EPSILON_FLOAT || cameraV.norm2() < Numeric::EPSILON_FLOAT ) {
        return;
    }

    cameraU.normalize(Numeric::EPSILON_FLOAT);
    cameraV.normalize(Numeric::EPSILON_FLOAT);
    axisU->copy(cameraU);
    axisV->copy(cameraV);
}

void
GlutDebugToolsMouseControl::rotateVectorAroundAxis(Vector3D *vector, const Vector3D &axis, float angleDegrees) {
    if ( vector == nullptr || axis.norm2() < Numeric::EPSILON_FLOAT || angleDegrees == 0.0f ) {
        return;
    }

    Matrix4x4 rotation = Matrix4x4::createRotationMatrix(angleDegrees * DEGREES_TO_RADIANS, axis);
    Vector3D rotated;
    rotation.transformPoint3D(*vector, rotated);
    vector->copy(rotated);
}

void
GlutDebugToolsMouseControl::applyInverseDebugRotationToRay(const GlutDebugToolsModel &model, Ray *ray) {
    if ( ray == nullptr ) {
        return;
    }

    const float angleAroundU = GLOBAL_render_glutDebugState.angleAroundViewportU;
    const float angleAroundV = GLOBAL_render_glutDebugState.angleAroundViewportV;
    if ( angleAroundU == 0.0f && angleAroundV == 0.0f ) {
        return;
    }

    const Vector3D pivot = sceneRotationPivot(model.scene);
    Vector3D axisU;
    Vector3D axisV;
    viewportAxesInWorld(model.scene, &axisU, &axisV);

    Vector3D translatedOrigin;
    translatedOrigin.subtraction(ray->position, pivot);
    rotateVectorAroundAxis(&translatedOrigin, axisU, -angleAroundU);
    rotateVectorAroundAxis(&translatedOrigin, axisV, -angleAroundV);
    ray->position.addition(pivot, translatedOrigin);

    rotateVectorAroundAxis(&ray->direction, axisU, -angleAroundU);
    rotateVectorAroundAxis(&ray->direction, axisV, -angleAroundV);
    ray->direction.normalize(Numeric::EPSILON_FLOAT);
}

void
GlutDebugToolsMouseControl::buildPickRay(const GlutDebugToolsModel &model, int x, int y, Ray *ray) {
    if ( ray == nullptr ) {
        return;
    }

    ray->position.set(0.0f, 0.0f, 0.0f);
    ray->direction.set(0.0f, 0.0f, 1.0f);

    if ( model.scene == nullptr || model.scene->camera == nullptr || model.width <= 0 || model.height <= 0 ) {
        return;
    }

    syncCameraToViewport(model);

    Camera *camera = model.scene->camera;
    Render::renderGetNearFar(camera, model.scene->geometryList);

    float nearDistance = camera->near;
    if ( nearDistance < Numeric::EPSILON_FLOAT ) {
        if ( camera->viewDistance > Numeric::EPSILON_FLOAT ) {
            nearDistance = camera->viewDistance / 100.0f;
        } else {
            nearDistance = 0.1f;
        }
    }

    const int pixelX = clampCoord(x, model.width);
    const int pixelY = clampCoord(y, model.height);

    const float normalizedU =
        (2.0f * ((static_cast<float>(pixelX) + 0.5f) / static_cast<float>(model.width))) - 1.0f;
    const float normalizedV =
        (2.0f * ((static_cast<float>(pixelY) + 0.5f) / static_cast<float>(model.height))) - 1.0f;

    // Point (u, v, n) on the near plane in camera coordinates.
    const float nearPlaneU = normalizedU * camera->pixelWidthTangent * nearDistance;
    const float nearPlaneV = normalizedV * camera->pixelHeightTangent * nearDistance;

    // Direction from eye to that near-plane point.
    const float uAtUnitDepth = nearPlaneU / nearDistance;
    const float vAtUnitDepth = nearPlaneV / nearDistance;

    ray->position = camera->eyePosition;
    ray->direction.combine3(camera->Z, uAtUnitDepth, camera->X, vAtUnitDepth, camera->Y);
    ray->direction.normalize(Numeric::EPSILON_FLOAT);

    applyInverseDebugRotationToRay(model, ray);
}

bool
GlutDebugToolsMouseControl::pickPatchAtMousePosition(
    const GlutDebugToolsModel &model,
    int x,
    int y,
    int *patchIndex)
{
    if ( patchIndex == nullptr ) {
        return false;
    }
    *patchIndex = -1;

    if ( model.scene == nullptr || model.scene->patchList == nullptr ) {
        return false;
    }

    Ray ray;
    buildPickRay(model, x, y, &ray);

    java::ArrayList<PatchHitCandidate> hitCandidates(model.scene->patchList->size());

    for ( int i = 0; i < model.scene->patchList->size(); i++ ) {
        Patch *patch = model.scene->patchList->get(i);
        if ( patch == nullptr ) {
            continue;
        }
        if ( patch->radianceData == nullptr || patch->radianceData->className != ElementTypes::ELEMENT_GALERKIN ) {
            continue;
        }

        float maxDistance = Numeric::HUGE_FLOAT_VALUE;
        RayHit hit;
        RayHit *intersection = patch->intersect(
            &ray,
            Numeric::EPSILON_FLOAT,
            &maxDistance,
            RayHitFlag::FRONT | RayHitFlag::BACK,
            &hit);
        if ( intersection != nullptr ) {
            const unsigned int flags = hit.getFlags();
            const bool frontFacing = (flags & RayHitFlag::FRONT) != 0;
            hitCandidates.add(PatchHitCandidate{i, maxDistance, frontFacing});
        }
    }

    if ( hitCandidates.size() <= 0 ) {
        return false;
    }

    float nearestFrontDistance = Numeric::HUGE_FLOAT_VALUE;
    int nearestFrontPatchIndex = -1;
    float nearestBackDistance = Numeric::HUGE_FLOAT_VALUE;
    int nearestBackPatchIndex = -1;

    for ( int i = 0; i < hitCandidates.size(); i++ ) {
        const PatchHitCandidate candidate = hitCandidates.get(i);
        if ( candidate.frontFacing ) {
            if ( candidate.distance < nearestFrontDistance ) {
                nearestFrontDistance = candidate.distance;
                nearestFrontPatchIndex = candidate.patchIndex;
            }
        } else if ( candidate.distance < nearestBackDistance ) {
            nearestBackDistance = candidate.distance;
            nearestBackPatchIndex = candidate.patchIndex;
        }
    }

    if ( nearestFrontPatchIndex >= 0 ) {
        *patchIndex = nearestFrontPatchIndex;
        return true;
    }

    if ( nearestBackPatchIndex < 0 ) {
        return false;
    }

    *patchIndex = nearestBackPatchIndex;
    return true;
}

bool
GlutDebugToolsMouseControl::handleMouseButton(
    int button,
    int state,
    int x,
    int y,
    GlutDebugToolsModel &model)
{
    if ( button != GLUT_LEFT_BUTTON ) {
        return false;
    }

    const int clampedX = clampCoord(x, model.width);
    const int clampedY = clampCoord(y, model.height);

    if ( state == GLUT_DOWN ) {
        const int modifiers = glutGetModifiers();
        globalPressWithShift = (modifiers & GLUT_ACTIVE_SHIFT) != 0;
        globalLeftButtonDown = true;
        globalDragging = false;
        globalPressX = clampedX;
        globalPressY = clampedY;
        globalLastX = clampedX;
        globalLastY = clampedY;
        return false;
    }

    if ( state != GLUT_UP || !globalLeftButtonDown ) {
        return false;
    }

    globalLeftButtonDown = false;
    const bool shouldSelectPatch = !globalDragging;
    globalDragging = false;

    if ( !shouldSelectPatch ) {
        return false;
    }

    int patchIndex = -1;
    if ( !pickPatchAtMousePosition(model, clampedX, clampedY, &patchIndex) ) {
        patchIndex = -1;
    }

    int *targetSelection = &GLOBAL_render_glutDebugState.primarySelectedPatch;
    const bool isPrimarySelection = !globalPressWithShift;
    if ( globalPressWithShift ) {
        targetSelection = &GLOBAL_render_glutDebugState.selectedSelectedPatch;
    }

    if ( !applyPatchSelection(patchIndex, targetSelection) ) {
        return false;
    }
    if ( isPrimarySelection ) {
        clampSelectedHierarchyLevel(model);
    }

    return true;
}

bool
GlutDebugToolsMouseControl::handleMouseMotion(int x, int y, GlutDebugToolsModel &model) {
    if ( !globalLeftButtonDown ) {
        return false;
    }

    const int clampedX = clampCoord(x, model.width);
    const int clampedY = clampCoord(y, model.height);

    const int deltaX = clampedX - globalLastX;
    const int deltaY = clampedY - globalLastY;
    globalLastX = clampedX;
    globalLastY = clampedY;

    if ( deltaX == 0 && deltaY == 0 ) {
        return false;
    }

    if ( !globalDragging ) {
        const int fromPressX = std::abs(clampedX - globalPressX);
        const int fromPressY = std::abs(clampedY - globalPressY);
        if ( fromPressX >= DRAG_START_THRESHOLD_PIXELS || fromPressY >= DRAG_START_THRESHOLD_PIXELS ) {
            globalDragging = true;
        }
    }

    if ( !globalDragging ) {
        return false;
    }

    GLOBAL_render_glutDebugState.angleAroundViewportV -= static_cast<float>(deltaX) * DRAG_ROTATION_DEGREES_PER_PIXEL;
    GLOBAL_render_glutDebugState.angleAroundViewportU += static_cast<float>(deltaY) * DRAG_ROTATION_DEGREES_PER_PIXEL;
    return true;
}
