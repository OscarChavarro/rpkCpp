#include "render/opengl/visualDebugTools/GlutDebugToolsMouseControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "common/linealAlgebra/Matrix4x4.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "material/RayHit.h"
#include "material/RayHitFlag.h"
#include "render/opengl/RenderOpenGL.h"
#include "render/opengl/visualDebugTools/GlutDebugPatchHierarchy.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "render/opengl/visualDebugTools/PatchHitCandidate.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "skin/Patch.h"

bool GlutDebugToolsMouseControl::leftButtonDown = false;
bool GlutDebugToolsMouseControl::dragging = false;
bool GlutDebugToolsMouseControl::pressWithShift = false;
int GlutDebugToolsMouseControl::pressX = 0;
int GlutDebugToolsMouseControl::pressY = 0;
int GlutDebugToolsMouseControl::lastX = 0;
int GlutDebugToolsMouseControl::lastY = 0;

bool
GlutDebugToolsMouseControl::applyPatchSelection(
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
GlutDebugToolsMouseControl::syncCameraToViewport(const GlutDebugToolsModel &model) {
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
    if ( model.debugState == nullptr ) {
        return;
    }

    const int maxHierarchyLevel = GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
        model.scene,
        model.debugState->primarySelectedPatch);

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
    if ( model.debugState == nullptr ) {
        return;
    }

    const float angleAroundU = model.debugState->angleAroundViewportU;
    const float angleAroundV = model.debugState->angleAroundViewportV;
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
    RenderOpenGL::renderGetNearFar(camera, model.scene->geometryList);

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
            hitCandidates.add(PatchHitCandidate(i, maxDistance, frontFacing));
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
        GlutDebugToolsMouseControl::pressWithShift = (modifiers & GLUT_ACTIVE_SHIFT) != 0;
        GlutDebugToolsMouseControl::leftButtonDown = true;
        GlutDebugToolsMouseControl::dragging = false;
        GlutDebugToolsMouseControl::pressX = clampedX;
        GlutDebugToolsMouseControl::pressY = clampedY;
        GlutDebugToolsMouseControl::lastX = clampedX;
        GlutDebugToolsMouseControl::lastY = clampedY;
        return false;
    }

    if ( state != GLUT_UP || !GlutDebugToolsMouseControl::leftButtonDown ) {
        return false;
    }

    GlutDebugToolsMouseControl::leftButtonDown = false;
    const bool shouldSelectPatch = !GlutDebugToolsMouseControl::dragging;
    GlutDebugToolsMouseControl::dragging = false;

    if ( !shouldSelectPatch ) {
        return false;
    }
    if ( model.debugState == nullptr ) {
        return false;
    }

    int patchIndex = -1;
    if ( !pickPatchAtMousePosition(model, clampedX, clampedY, &patchIndex) ) {
        patchIndex = -1;
    }

    int *targetSelection = &model.debugState->primarySelectedPatch;
    const bool isPrimarySelection = !GlutDebugToolsMouseControl::pressWithShift;
    if ( GlutDebugToolsMouseControl::pressWithShift ) {
        targetSelection = &model.debugState->selectedSelectedPatch;
    }

    if ( !GlutDebugToolsMouseControl::applyPatchSelection(patchIndex, targetSelection) ) {
        return false;
    }
    if ( isPrimarySelection ) {
        clampSelectedHierarchyLevel(model);
    }

    return true;
}

bool
GlutDebugToolsMouseControl::handleMouseMotion(int x, int y, GlutDebugToolsModel &model) {
    if ( !GlutDebugToolsMouseControl::leftButtonDown ) {
        return false;
    }

    const int clampedX = clampCoord(x, model.width);
    const int clampedY = clampCoord(y, model.height);

    const int deltaX = clampedX - GlutDebugToolsMouseControl::lastX;
    const int deltaY = clampedY - GlutDebugToolsMouseControl::lastY;
    GlutDebugToolsMouseControl::lastX = clampedX;
    GlutDebugToolsMouseControl::lastY = clampedY;

    if ( deltaX == 0 && deltaY == 0 ) {
        return false;
    }

    if ( !GlutDebugToolsMouseControl::dragging ) {
        const int fromPressX = static_cast<int>(java::Math::abs(static_cast<float>(clampedX - GlutDebugToolsMouseControl::pressX)));
        const int fromPressY = static_cast<int>(java::Math::abs(static_cast<float>(clampedY - GlutDebugToolsMouseControl::pressY)));
        if ( fromPressX >= DRAG_START_THRESHOLD_PIXELS || fromPressY >= DRAG_START_THRESHOLD_PIXELS ) {
            GlutDebugToolsMouseControl::dragging = true;
        }
    }

    if ( !GlutDebugToolsMouseControl::dragging ) {
        return false;
    }
    if ( model.debugState == nullptr ) {
        return false;
    }

    model.debugState->angleAroundViewportV -= static_cast<float>(deltaX) * DRAG_ROTATION_DEGREES_PER_PIXEL;
    model.debugState->angleAroundViewportU += static_cast<float>(deltaY) * DRAG_ROTATION_DEGREES_PER_PIXEL;
    return true;
}
