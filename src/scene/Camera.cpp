#include "common/error.h"
#include "scene/Camera.h"

Camera::Camera(): background() {
    eyePosition = Vector3D{};
    lookPosition = Vector3D{};
    upDirection = Vector3D{};
    viewDistance = 0.0f;
    fieldOfVision = 0.0f;
    horizontalFov = 0.0f;
    verticalFov = 0.0f;
    near = 0.0f;
    far = 0.0f;
    xSize = 0;
    ySize = 0;
    X = Vector3D{};
    Y = Vector3D{};
    Z = Vector3D{};
    background = ColorRgb{};
    changed = 0;
    pixelWidth = 0.0f;
    pixelHeight = 0.0f;
    pixelWidthTangent = 0.0f;
    pixelHeightTangent = 0.0f;
}

static void
cameraComputeClippingPlanes(Camera *camera) {
    float x = camera->pixelWidthTangent * camera->viewDistance; // Half the width of the virtual screen in 3D space
    float y = camera->pixelHeightTangent * camera->viewDistance; // Half the height of the virtual screen
    Vector3D vScreen[4];

    vScreen[0].combine3(camera->lookPosition, x, camera->X, -y, camera->Y); // Upper right corner: Y axis positions down!
    vScreen[1].combine3(camera->lookPosition, x, camera->X, y, camera->Y); // Lower right
    vScreen[2].combine3(camera->lookPosition, -x, camera->X, y, camera->Y); // Lower left
    vScreen[3].combine3(camera->lookPosition, -x, camera->X, -y, camera->Y); // Upper left

    for ( int i = 0; i < 4; i++ ) {
        camera->viewPlanes[i].normal.tripleCrossProduct(vScreen[(i + 1) % 4], camera->eyePosition, vScreen[i]);
        camera->viewPlanes[i].normal.normalize(EPSILON_FLOAT);
        camera->viewPlanes[i].d = -camera->viewPlanes[i].normal.dotProduct(camera->eyePosition);
    }
}

/**
Computes camera coordinate system and horizontal and vertical fov depending
on filled in fov value and aspect ratio of the view window. Returns
nullptr if this fails, and a pointer to the camera arg if success
*/
static Camera *
cameraComplete(Camera *camera) {
    float n;

    // Compute viewing direction ==> Z axis of eye coordinate system
    camera->Z.subtraction(camera->lookPosition, camera->eyePosition);

    // Distance from virtual camera position to focus point
    camera->viewDistance = camera->Z.norm();
    if ( camera->viewDistance < EPSILON ) {
        logError("SetCamera", "eye point and look-point coincide");
        return nullptr;
    }
    camera->Z.inverseScaledCopy(camera->viewDistance, camera->Z, EPSILON_FLOAT);

    // camera->X is a direction pointing to the right in the window
    camera->X.crossProduct(camera->Z, camera->upDirection);
    n = camera->X.norm();
    if ( n < EPSILON ) {
        logError("SetCamera", "up-direction and viewing direction coincide");
        return nullptr;
    }
    camera->X.inverseScaledCopy(n, camera->X, EPSILON_FLOAT);

    // camera->Y is a direction pointing down in the window
    camera->Y.crossProduct(camera->Z, camera->X);
    camera->Y.normalize(EPSILON_FLOAT);

    // Compute horizontal and vertical field of view angle from the specified one
    if ( camera->xSize < camera->ySize ) {
        camera->horizontalFov = camera->fieldOfVision;
        camera->verticalFov = (float)java::Math::atan(tan(camera->fieldOfVision * M_PI / 180.0) *
                                               (float)camera->ySize / (float) camera->xSize) * 180.0f / (float)M_PI;
    } else {
        camera->verticalFov = camera->fieldOfVision;
        camera->horizontalFov = (float)java::Math::atan(tan(camera->fieldOfVision * M_PI / 180.0) *
                                                 (float)camera->xSize / (float)camera->ySize) * 180.0f / (float)M_PI;
    }

    // Default near and far clipping plane distance, will be set to a more reasonable
    // value when setting the camera for rendering
    camera->near = EPSILON_FLOAT;
    camera->far = 2.0f * camera->viewDistance;

    // Compute some extra frequently used quantities
    camera->pixelWidthTangent = (float)java::Math::tan(camera->horizontalFov * M_PI / 180.0);
    camera->pixelHeightTangent = (float)java::Math::tan(camera->verticalFov * M_PI / 180.0);

    camera->pixelWidth = 2.0f * camera->pixelWidthTangent / (float) (camera->xSize);
    camera->pixelHeight = 2.0f * camera->pixelHeightTangent / (float) (camera->ySize);

    cameraComputeClippingPlanes(camera);

    return camera;
}

/**
Sets virtual camera position, focus point, up-direction, field of view
(in degrees), horizontal and vertical window resolution and window
inBackground. Returns (CAMERA *)nullptr if eye point and focus point coincide or
viewing direction is equal to the up-direction
*/
void
Camera::set(
    const Vector3D *inEyePosition,
    const Vector3D *inLoopPosition,
    const Vector3D *inUpDirection,
    const float inFieldOfVision,
    const int inXSize,
    const int inYSize,
    const ColorRgb *inBackground)
{
    eyePosition = *inEyePosition;
    lookPosition = *inLoopPosition;
    upDirection = *inUpDirection;
    fieldOfVision = inFieldOfVision;
    xSize = inXSize;
    ySize = inYSize;
    background = *inBackground;
    changed = true;
    cameraComplete(this);
}

/**
Sets virtual camera position, focus point, up-direction, field of view
(in degrees), horizontal and vertical window resolution and window
background. Returns (CAMERA *)nullptr if eye point and focus point coincide or
viewing direction is equal to the up-direction
*/
void
Camera::setEyePosition(float x, float y, float z) {
    Vector3D newEyePosition;
    newEyePosition.set(x, y, z);
    set(&newEyePosition, &lookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setLookPosition(float x, float y, float z) {
    Vector3D newLookPosition;
    newLookPosition.set(x, y, z);
    set(&eyePosition, &newLookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setUpDirection(float x, float y, float z) {
    Vector3D newUpDirection;
    newUpDirection.set(x, y, z);
    set(&eyePosition, &lookPosition, &newUpDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setFieldOfView(float fieldOfView) {
    set(&eyePosition, &lookPosition, &upDirection, fieldOfView, xSize, ySize, &background);
}
