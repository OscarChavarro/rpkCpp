#include "java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsMouseControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/render/opengl/RenderOpenGL.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugPatchHierarchy.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/PatchHitCandidate.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

bool GlutDebugToolsMouseControl::leftButtonDown = false;
bool GlutDebugToolsMouseControl::rightButtonDown = false;
bool GlutDebugToolsMouseControl::dragging = false;
bool GlutDebugToolsMouseControl::pressWithShift = false;
int GlutDebugToolsMouseControl::activeDragButton = -1;
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
    if ( model.selectedHierarchyLevel < 0 ) {
        model.selectedHierarchyLevel = 0;
    }
}

Vector3Df
GlutDebugToolsMouseControl::sceneRotationPivot(const Scene *scene) {
    if ( scene == nullptr ) {
        return Vector3Df(0.0F, 0.0F, 0.0F);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        RpkAxisAlignedBoundingBox sceneBounds;
        RpkGeometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3Df(0.0F, 0.0F, 0.0F);
}

void
GlutDebugToolsMouseControl::viewportAxesInWorld(const Scene *scene, Vector3Df *axisU, Vector3Df *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    *axisU = Vector3Df(1.0F, 0.0F, 0.0F);
    *axisV = Vector3Df(0.0F, 1.0F, 0.0F);

    if ( scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    const Camera *camera = scene->camera;

    Vector3Df cameraU;
    cameraU = camera->X;
    Vector3Df cameraV;
    cameraV = camera->Y;

    if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT || cameraV.lengthSquared() < Numeric::EPSILON_FLOAT ) {
        Vector3Df viewDirection;
        viewDirection = camera->lookPosition.subtract(camera->eyePosition);
        if ( viewDirection.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        viewDirection = viewDirection.normalized(Numeric::EPSILON_FLOAT);

        Vector3Df upDirection;
        upDirection = camera->upDirection;
        if ( upDirection.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            upDirection = Vector3Df(0.0F, 0.0F, 1.0F);
        } else {
            upDirection = upDirection.normalized(Numeric::EPSILON_FLOAT);
        }

        cameraU = viewDirection.crossProduct(upDirection);
        if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            upDirection = Vector3Df(0.0F, 1.0F, 0.0F);
            cameraU = viewDirection.crossProduct(upDirection);
        }
        if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            return;
        }

        cameraU = cameraU.normalized(Numeric::EPSILON_FLOAT);
        cameraV = viewDirection.crossProduct(cameraU);
    }

    if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT || cameraV.lengthSquared() < Numeric::EPSILON_FLOAT ) {
        return;
    }

    cameraU = cameraU.normalized(Numeric::EPSILON_FLOAT);
    cameraV = cameraV.normalized(Numeric::EPSILON_FLOAT);
    *axisU = cameraU;
    *axisV = cameraV;
}

void
GlutDebugToolsMouseControl::rotateVectorAroundAxis(Vector3Df *vector, const Vector3Df &axis, float angleDegrees) {
    if ( vector == nullptr || axis.lengthSquared() < Numeric::EPSILON_FLOAT || angleDegrees == 0.0F ) {
        return;
    }

    const Matrix4x4f rotation = Matrix4x4f().axisRotation(angleDegrees * DEGREES_TO_RADIANS, axis);
    *vector = rotation.withoutTranslation().multiply(*vector);
}

bool
GlutDebugToolsMouseControl::dollyCameraAlongFront(GlutDebugToolsModel &model, int deltaY) {
    if ( deltaY == 0 || model.scene == nullptr || model.scene->camera == nullptr ) {
        return false;
    }

    syncCameraToViewport(model);

    Camera *camera = model.scene->camera;
    Vector3Df viewDirection;
    viewDirection = camera->lookPosition.subtract(camera->eyePosition);
    const float viewDistance = viewDirection.length();
    if ( viewDistance < Numeric::EPSILON_FLOAT ) {
        return false;
    }
    viewDirection = viewDirection.multiplyByInverse(viewDistance, Numeric::EPSILON_FLOAT);

    const float translationDistance =
        -static_cast<float>(deltaY) * viewDistance * DRAG_DOLLY_VIEW_DISTANCE_FACTOR_PER_PIXEL;
    if ( java::Math::abs(translationDistance) < Numeric::EPSILON_FLOAT ) {
        return false;
    }

    Vector3Df translation;
    translation = viewDirection.multiply(translationDistance);

    Vector3Df newEyePosition;
    newEyePosition = camera->eyePosition.add(translation);
    Vector3Df newLookPosition;
    newLookPosition = camera->lookPosition.add(translation);

    const int viewportWidth = model.width > 0 ? model.width : camera->xSize;
    const int viewportHeight = model.height > 0 ? model.height : camera->ySize;
    camera->set(
        &newEyePosition,
        &newLookPosition,
        &camera->upDirection,
        camera->fieldOfVision,
        viewportWidth,
        viewportHeight,
        &camera->background);
    return true;
}

void
GlutDebugToolsMouseControl::applyInverseDebugRotationToRay(const GlutDebugToolsModel &model, RpkRay *ray) {
    if ( ray == nullptr ) {
        return;
    }
    if ( model.debugState == nullptr ) {
        return;
    }

    const float angleAroundU = model.debugState->angleAroundViewportU;
    const float angleAroundV = model.debugState->angleAroundViewportV;
    if ( angleAroundU == 0.0F && angleAroundV == 0.0F ) {
        return;
    }

    const Vector3Df pivot = sceneRotationPivot(model.scene);
    Vector3Df axisU;
    Vector3Df axisV;
    viewportAxesInWorld(model.scene, &axisU, &axisV);

    Vector3Df translatedOrigin;
    translatedOrigin = ray->position.subtract(pivot);
    rotateVectorAroundAxis(&translatedOrigin, axisU, -angleAroundU);
    rotateVectorAroundAxis(&translatedOrigin, axisV, -angleAroundV);
    ray->position = pivot.add(translatedOrigin);

    rotateVectorAroundAxis(&ray->direction, axisU, -angleAroundU);
    rotateVectorAroundAxis(&ray->direction, axisV, -angleAroundV);
    ray->direction = ray->direction.normalized(Numeric::EPSILON_FLOAT);
}

void
GlutDebugToolsMouseControl::buildPickRay(const GlutDebugToolsModel &model, int x, int y, RpkRay *ray) {
    if ( ray == nullptr ) {
        return;
    }

    ray->position = Vector3Df(0.0F, 0.0F, 0.0F);
    ray->direction = Vector3Df(0.0F, 0.0F, 1.0F);

    if ( model.scene == nullptr || model.scene->camera == nullptr || model.width <= 0 || model.height <= 0 ) {
        return;
    }

    syncCameraToViewport(model);

    Camera *camera = model.scene->camera;
    RenderOpenGL::renderGetNearFar(camera, model.scene->geometryList);

    float nearDistance = camera->near;
    if ( nearDistance < Numeric::EPSILON_FLOAT ) {
        if ( camera->viewDistance > Numeric::EPSILON_FLOAT ) {
            nearDistance = camera->viewDistance / 100.0F;
        } else {
            nearDistance = 0.1F;
        }
    }

    const int pixelX = clampCoord(x, model.width);
    const int pixelY = clampCoord(y, model.height);

    const float normalizedU =
        (2.0F * ((static_cast<float>(pixelX) + 0.5F) / static_cast<float>(model.width))) - 1.0F;
    const float normalizedV =
        (2.0F * ((static_cast<float>(pixelY) + 0.5F) / static_cast<float>(model.height))) - 1.0F;

    // Point (u, v, n) on the near plane in camera coordinates.
    const float nearPlaneU = normalizedU * camera->pixelWidthTangent * nearDistance;
    const float nearPlaneV = normalizedV * camera->pixelHeightTangent * nearDistance;

    // Direction from eye to that near-plane point.
    const float uAtUnitDepth = nearPlaneU / nearDistance;
    const float vAtUnitDepth = nearPlaneV / nearDistance;

    ray->position = camera->eyePosition;
    ray->direction = camera->Z.add(camera->X.multiply(uAtUnitDepth)).add(camera->Y.multiply(vAtUnitDepth));
    ray->direction = ray->direction.normalized(Numeric::EPSILON_FLOAT);

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

    RpkRay ray;
    buildPickRay(model, x, y, &ray);

    java::ArrayList<PatchHitCandidate> hitCandidates(model.scene->patchList->size());

    for ( int i = 0; i < model.scene->patchList->size(); i++ ) {
        Patch *patch = model.scene->patchList->get(i);
        if ( patch == nullptr ) {
            continue;
        }
        if ( GalerkinRadianceMethod::galerkinState.boundElement(patch) == nullptr || GalerkinRadianceMethod::galerkinState.boundElement(patch)->className != ElementTypes::ELEMENT_GALERKIN ) {
            continue;
        }

        float maxDistance = java::Float::MAX_VALUE;
        RpkRayHit hit;
        const RayExclusionSet noExclusions;
        RpkRayHit *intersection = patch->intersect(
            &ray,
            Numeric::EPSILON_FLOAT,
            &maxDistance,
            RayHitFlag::FRONT | RayHitFlag::BACK,
            &hit,
            noExclusions);
        if ( intersection != nullptr ) {
            const unsigned int flags = hit.getFlags();
            const bool frontFacing = (flags & RayHitFlag::FRONT) != 0;
            hitCandidates.add(PatchHitCandidate(i, maxDistance, frontFacing));
        }
    }

    if ( hitCandidates.size() <= 0 ) {
        return false;
    }

    float nearestFrontDistance = java::Float::MAX_VALUE;
    int nearestFrontPatchIndex = -1;
    float nearestBackDistance = java::Float::MAX_VALUE;
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
    const bool isLeftButton = button == GLUT_LEFT_BUTTON;
    const bool isRightButton = button == GLUT_RIGHT_BUTTON;
    if ( !isLeftButton && !isRightButton ) {
        return false;
    }

    const int clampedX = clampCoord(x, model.width);
    const int clampedY = clampCoord(y, model.height);

    if ( state == GLUT_DOWN ) {
        GlutDebugToolsMouseControl::leftButtonDown = isLeftButton;
        GlutDebugToolsMouseControl::rightButtonDown = isRightButton;
        GlutDebugToolsMouseControl::activeDragButton = button;
        if ( isLeftButton ) {
            const int modifiers = glutGetModifiers();
            GlutDebugToolsMouseControl::pressWithShift = (modifiers & GLUT_ACTIVE_SHIFT) != 0;
        } else {
            GlutDebugToolsMouseControl::pressWithShift = false;
        }
        GlutDebugToolsMouseControl::dragging = false;
        GlutDebugToolsMouseControl::pressX = clampedX;
        GlutDebugToolsMouseControl::pressY = clampedY;
        GlutDebugToolsMouseControl::lastX = clampedX;
        GlutDebugToolsMouseControl::lastY = clampedY;
        return false;
    }

    if ( state != GLUT_UP ) {
        return false;
    }

    if ( isRightButton ) {
        if ( !GlutDebugToolsMouseControl::rightButtonDown ) {
            return false;
        }
        GlutDebugToolsMouseControl::rightButtonDown = false;
        if ( GlutDebugToolsMouseControl::activeDragButton == GLUT_RIGHT_BUTTON ) {
            GlutDebugToolsMouseControl::activeDragButton = -1;
            GlutDebugToolsMouseControl::dragging = false;
        }
        return false;
    }

    if ( !GlutDebugToolsMouseControl::leftButtonDown ) {
        return false;
    }

    GlutDebugToolsMouseControl::leftButtonDown = false;
    const bool shouldSelectPatch =
        !GlutDebugToolsMouseControl::dragging
        && GlutDebugToolsMouseControl::activeDragButton == GLUT_LEFT_BUTTON;
    if ( GlutDebugToolsMouseControl::activeDragButton == GLUT_LEFT_BUTTON ) {
        GlutDebugToolsMouseControl::activeDragButton = -1;
        GlutDebugToolsMouseControl::dragging = false;
    }

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
    if ( GlutDebugToolsMouseControl::activeDragButton != GLUT_LEFT_BUTTON
         && GlutDebugToolsMouseControl::activeDragButton != GLUT_RIGHT_BUTTON ) {
        return false;
    }
    if ( GlutDebugToolsMouseControl::activeDragButton == GLUT_LEFT_BUTTON
         && !GlutDebugToolsMouseControl::leftButtonDown ) {
        return false;
    }
    if ( GlutDebugToolsMouseControl::activeDragButton == GLUT_RIGHT_BUTTON
         && !GlutDebugToolsMouseControl::rightButtonDown ) {
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

    if ( GlutDebugToolsMouseControl::activeDragButton == GLUT_LEFT_BUTTON ) {
        if ( model.debugState == nullptr ) {
            return false;
        }
        model.debugState->angleAroundViewportV -= static_cast<float>(deltaX) * DRAG_ROTATION_DEGREES_PER_PIXEL;
        model.debugState->angleAroundViewportU += static_cast<float>(deltaY) * DRAG_ROTATION_DEGREES_PER_PIXEL;
        return true;
    }

    return dollyCameraAlongFront(model, deltaY);
}
