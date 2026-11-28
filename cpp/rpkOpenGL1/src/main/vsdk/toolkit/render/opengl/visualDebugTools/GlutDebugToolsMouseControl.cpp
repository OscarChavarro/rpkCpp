#include "java/lang/Double.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsMouseControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
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
         camera->pixelWidth > Numeric::EPSILON &&
         camera->pixelHeight > Numeric::EPSILON ) {
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

Vector3Dd
GlutDebugToolsMouseControl::sceneRotationPivot(const Scene *scene) {
    if ( scene == nullptr ) {
        return Vector3Dd(0.0, 0.0, 0.0);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        RpkAxisAlignedBoundingBox sceneBounds;
        RpkGeometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3Dd(0.0, 0.0, 0.0);
}

void
GlutDebugToolsMouseControl::viewportAxesInWorld(const Scene *scene, Vector3Dd *axisU, Vector3Dd *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    *axisU = Vector3Dd(1.0, 0.0, 0.0);
    *axisV = Vector3Dd(0.0, 1.0, 0.0);

    if ( scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    const Camera *camera = scene->camera;

    Vector3Dd cameraU;
    cameraU = camera->X;
    Vector3Dd cameraV;
    cameraV = camera->Y;

    if ( cameraU.lengthSquared() < Numeric::EPSILON || cameraV.lengthSquared() < Numeric::EPSILON ) {
        Vector3Dd viewDirection;
        viewDirection = camera->lookPosition.subtract(camera->eyePosition);
        if ( viewDirection.lengthSquared() < Numeric::EPSILON ) {
            return;
        }
        viewDirection = viewDirection.normalized(Numeric::EPSILON);

        Vector3Dd upDirection;
        upDirection = camera->upDirection;
        if ( upDirection.lengthSquared() < Numeric::EPSILON ) {
            upDirection = Vector3Dd(0.0, 0.0, 1.0);
        } else {
            upDirection = upDirection.normalized(Numeric::EPSILON);
        }

        cameraU = viewDirection.crossProduct(upDirection);
        if ( cameraU.lengthSquared() < Numeric::EPSILON ) {
            upDirection = Vector3Dd(0.0, 1.0, 0.0);
            cameraU = viewDirection.crossProduct(upDirection);
        }
        if ( cameraU.lengthSquared() < Numeric::EPSILON ) {
            return;
        }

        cameraU = cameraU.normalized(Numeric::EPSILON);
        cameraV = viewDirection.crossProduct(cameraU);
    }

    if ( cameraU.lengthSquared() < Numeric::EPSILON || cameraV.lengthSquared() < Numeric::EPSILON ) {
        return;
    }

    cameraU = cameraU.normalized(Numeric::EPSILON);
    cameraV = cameraV.normalized(Numeric::EPSILON);
    *axisU = cameraU;
    *axisV = cameraV;
}

void
GlutDebugToolsMouseControl::rotateVectorAroundAxis(Vector3Dd *vector, const Vector3Dd &axis, double angleDegrees) {
    if ( vector == nullptr || axis.lengthSquared() < Numeric::EPSILON || angleDegrees == 0.0 ) {
        return;
    }

    const Matrix4x4d rotation = Matrix4x4d().axisRotation(angleDegrees * DEGREES_TO_RADIANS, axis);
    *vector = rotation.withoutTranslation().multiply(*vector);
}

bool
GlutDebugToolsMouseControl::dollyCameraAlongFront(GlutDebugToolsModel &model, int deltaY) {
    if ( deltaY == 0 || model.scene == nullptr || model.scene->camera == nullptr ) {
        return false;
    }

    syncCameraToViewport(model);

    Camera *camera = model.scene->camera;
    Vector3Dd viewDirection;
    viewDirection = camera->lookPosition.subtract(camera->eyePosition);
    const double viewDistance = viewDirection.length();
    if ( viewDistance < Numeric::EPSILON ) {
        return false;
    }
    viewDirection = viewDirection.multiplyByInverse(viewDistance, Numeric::EPSILON);

    const double translationDistance =
        -static_cast<double>(deltaY) * viewDistance * DRAG_DOLLY_VIEW_DISTANCE_FACTOR_PER_PIXEL;
    if ( java::Math::abs(translationDistance) < Numeric::EPSILON ) {
        return false;
    }

    Vector3Dd translation;
    translation = viewDirection.multiply(translationDistance);

    Vector3Dd newEyePosition;
    newEyePosition = camera->eyePosition.add(translation);
    Vector3Dd newLookPosition;
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
GlutDebugToolsMouseControl::applyInverseDebugRotationToRay(const GlutDebugToolsModel &model, Ray *ray) {
    if ( ray == nullptr ) {
        return;
    }
    if ( model.debugState == nullptr ) {
        return;
    }

    const double angleAroundU = model.debugState->angleAroundViewportU;
    const double angleAroundV = model.debugState->angleAroundViewportV;
    if ( angleAroundU == 0.0 && angleAroundV == 0.0 ) {
        return;
    }

    const Vector3Dd pivot = sceneRotationPivot(model.scene);
    Vector3Dd axisU;
    Vector3Dd axisV;
    viewportAxesInWorld(model.scene, &axisU, &axisV);

    Vector3Dd translatedOrigin;
    translatedOrigin = ray->getOrigin().subtract(pivot);
    rotateVectorAroundAxis(&translatedOrigin, axisU, -angleAroundU);
    rotateVectorAroundAxis(&translatedOrigin, axisV, -angleAroundV);
    ray->setOrigin(pivot.add(translatedOrigin));

    Vector3Dd direction = ray->getDirection();
    rotateVectorAroundAxis(&direction, axisU, -angleAroundU);
    rotateVectorAroundAxis(&direction, axisV, -angleAroundV);
    ray->setDirection(direction.normalized(Numeric::EPSILON));
}

void
GlutDebugToolsMouseControl::buildPickRay(const GlutDebugToolsModel &model, int x, int y, Ray *ray) {
    if ( ray == nullptr ) {
        return;
    }

    ray->setOrigin(Vector3Dd(0.0, 0.0, 0.0));
    ray->setDirection(Vector3Dd(0.0, 0.0, 1.0));

    if ( model.scene == nullptr || model.scene->camera == nullptr || model.width <= 0 || model.height <= 0 ) {
        return;
    }

    syncCameraToViewport(model);

    Camera *camera = model.scene->camera;
    RenderOpenGL::renderGetNearFar(camera, model.scene->geometryList);

    double nearDistance = camera->near;
    if ( nearDistance < Numeric::EPSILON ) {
        if ( camera->viewDistance > Numeric::EPSILON ) {
            nearDistance = camera->viewDistance / 100.0;
        } else {
            nearDistance = 0.1;
        }
    }

    const int pixelX = clampCoord(x, model.width);
    const int pixelY = clampCoord(y, model.height);

    const double normalizedU =
        (2.0 * ((static_cast<double>(pixelX) + 0.5) / static_cast<double>(model.width))) - 1.0;
    const double normalizedV =
        (2.0 * ((static_cast<double>(pixelY) + 0.5) / static_cast<double>(model.height))) - 1.0;

    // Point (u, v, n) on the near plane in camera coordinates.
    const double nearPlaneU = normalizedU * camera->pixelWidthTangent * nearDistance;
    const double nearPlaneV = normalizedV * camera->pixelHeightTangent * nearDistance;

    // Direction from eye to that near-plane point.
    const double uAtUnitDepth = nearPlaneU / nearDistance;
    const double vAtUnitDepth = nearPlaneV / nearDistance;

    ray->setOrigin(camera->eyePosition);
    ray->setDirection(camera->Z.add(camera->X.multiply(uAtUnitDepth)).add(camera->Y.multiply(vAtUnitDepth)));
    ray->setDirection(ray->getDirection().normalized(Numeric::EPSILON));

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
        if ( GalerkinRadianceMethod::galerkinState.boundElement(patch) == nullptr || GalerkinRadianceMethod::galerkinState.boundElement(patch)->className != ElementTypes::ELEMENT_GALERKIN ) {
            continue;
        }

        double maxDistance = java::Double::MAX_VALUE;
        RpkRayHit hit;
        const RayExclusionSet noExclusions;
        RpkRayHit *intersection = patch->intersect(
            &ray,
            Numeric::EPSILON,
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

    double nearestFrontDistance = java::Double::MAX_VALUE;
    int nearestFrontPatchIndex = -1;
    double nearestBackDistance = java::Double::MAX_VALUE;
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
        const int fromPressX = static_cast<int>(java::Math::abs(static_cast<double>(clampedX - GlutDebugToolsMouseControl::pressX)));
        const int fromPressY = static_cast<int>(java::Math::abs(static_cast<double>(clampedY - GlutDebugToolsMouseControl::pressY)));
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
        model.debugState->angleAroundViewportV -= static_cast<double>(deltaX) * DRAG_ROTATION_DEGREES_PER_PIXEL;
        model.debugState->angleAroundViewportU += static_cast<double>(deltaY) * DRAG_ROTATION_DEGREES_PER_PIXEL;
        return true;
    }

    return dollyCameraAlongFront(model, deltaY);
}
