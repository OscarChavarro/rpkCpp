#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/scene/Camera.h"

Camera::Camera(): background() {
    eyePosition = Vector3D{};
    lookPosition = Vector3D{};
    upDirection = Vector3D{};
    viewDistance = 0.0F;
    fieldOfVision = 0.0F;
    horizontalFov = 0.0F;
    verticalFov = 0.0F;
    near = 0.0F;
    far = 0.0F;
    xSize = 0;
    ySize = 0;
    X = Vector3D{};
    Y = Vector3D{};
    Z = Vector3D{};
    background = ColorRgbMutable{};
    changed = 0;
    pixelWidth = 0.0F;
    pixelHeight = 0.0F;
    pixelWidthTangent = 0.0F;
    pixelHeightTangent = 0.0F;
}

/**
Sets the near and far clipping distances from the scene bounds projected onto
the viewing direction (Z axis), with 2% extra distance on both sides and
fallbacks when the range is not in front of the eye. Every software and
hardware renderer needs these before rendering, including the ID renders of
the importance computations, which run before any image is drawn.
*/
void
Camera::fitClippingPlanes(const RpkAxisAlignedBoundingBox &sceneBounds) {
    Vector3D minimum;
    Vector3D maximum;
    Vector3D d;

    minimum.set(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
    maximum.set(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());

    far = -Numeric::HUGE_FLOAT_VALUE;
    near = Numeric::HUGE_FLOAT_VALUE;
    for ( int i = 0; i <= 1; i++ ) {
        for ( int j = 0; j <= 1; j++ ) {
            for ( int k = 0; k <= 1; k++ ) {
                d.set(i ? maximum.x : minimum.x,
                      j ? maximum.y : minimum.y,
                      k ? maximum.z : minimum.z);

                d.subtraction(d, eyePosition);
                float z = d.dotProduct(Z);

                if ( z > far ) {
                    far = z;
                }
                if ( z < near ) {
                    near = z;
                }
            }
        }
    }

    // Take 2% extra distance for near as well as far clipping plane
    far += 0.02F * (far);
    near -= 0.02F * (near);
    if ( far < Numeric::EPSILON ) {
        far = viewDistance;
    }
    if ( near < Numeric::EPSILON ) {
        near = viewDistance / 100.0F;
    }
}

void
Camera::computeClippingPlanes() {
    float x = pixelWidthTangent * viewDistance; // Half the width of the virtual screen in 3D space
    float y = pixelHeightTangent * viewDistance; // Half the height of the virtual screen
    Vector3D vScreen[4];

    vScreen[0].combine3(lookPosition, x, X, -y, Y); // Upper right corner: Y axis positions down!
    vScreen[1].combine3(lookPosition, x, X, y, Y); // Lower right
    vScreen[2].combine3(lookPosition, -x, X, y, Y); // Lower left
    vScreen[3].combine3(lookPosition, -x, X, -y, Y); // Upper left

    for ( int i = 0; i < 4; i++ ) {
        viewPlanes[i].normal.tripleCrossProduct(vScreen[(i + 1) % 4], eyePosition, vScreen[i]);
        viewPlanes[i].normal.normalize(Numeric::EPSILON_FLOAT);
        viewPlanes[i].d = -viewPlanes[i].normal.dotProduct(eyePosition);
    }
}

/**
Computes camera coordinate system and horizontal and vertical fov depending
on filled in fov value and aspect ratio of the view window. Returns
nullptr if this fails, and a pointer to the camera arg if success
*/
Camera *
Camera::complete() {
    // Compute viewing direction ==> Z axis of eye coordinate system
    Z.subtraction(lookPosition, eyePosition);

    // Distance from virtual camera position to focus point
    viewDistance = Z.norm();
    if ( viewDistance < Numeric::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "eye point and look-point coincide");
        return nullptr;
    }
    Z.inverseScaledCopy(viewDistance, Z, Numeric::EPSILON_FLOAT);

    // camera->X is a direction pointing to the right in the window
    X.crossProduct(Z, upDirection);
    const float n = X.norm();
    if ( n < Numeric::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "up-direction and viewing direction coincide");
        return nullptr;
    }
    X.inverseScaledCopy(n, X, Numeric::EPSILON_FLOAT);

    // camera->Y is a direction pointing down in the window
    Y.crossProduct(Z, X);
    Y.normalize(Numeric::EPSILON_FLOAT);

    // Compute horizontal and vertical field of view angle from the specified one
    if ( xSize < ySize ) {
        horizontalFov = fieldOfVision;
        verticalFov = static_cast<float>(java::Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                  static_cast<float>(ySize) / static_cast<float>(xSize))) * 180.0F / static_cast<float>(M_PI);
    } else {
        verticalFov = fieldOfVision;
        horizontalFov = static_cast<float>(java::Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                    static_cast<float>(xSize) / static_cast<float>(ySize))) * 180.0F / static_cast<float>(M_PI);
    }

    // Default near and far clipping plane distance, will be set to a more reasonable
    // value when setting the camera for rendering
    near = Numeric::EPSILON_FLOAT;
    far = 2.0F * viewDistance;

    // Compute some extra frequently used quantities
    pixelWidthTangent = static_cast<float>(java::Math::tan(horizontalFov * M_PI / 180.0));
    pixelHeightTangent = static_cast<float>(java::Math::tan(verticalFov * M_PI / 180.0));

    pixelWidth = 2.0F * pixelWidthTangent / static_cast<float>(xSize);
    pixelHeight = 2.0F * pixelHeightTangent / static_cast<float>(ySize);

    computeClippingPlanes();

    return this;
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
    const ColorRgbMutable *inBackground)
{
    eyePosition = *inEyePosition;
    lookPosition = *inLoopPosition;
    upDirection = *inUpDirection;
    fieldOfVision = inFieldOfVision;
    xSize = inXSize;
    ySize = inYSize;
    background = *inBackground;
    changed = true;
    complete();
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

void
Camera::transformBoundingBox(
    const RpkAxisAlignedBoundingBox &sourceBoundingBox,
    const Matrix4x4 &transform,
    RpkAxisAlignedBoundingBox *transformedBoundingBox)
{
    if ( transformedBoundingBox == nullptr ) {
        return;
    }

    Vector3D corners[8];
    sourceBoundingBox.corners(corners);

    *transformedBoundingBox = RpkAxisAlignedBoundingBox{};
    for ( int i = 0; i < 8; i++ ) {
        transform.transformPoint3D(corners[i], corners[i]);
        transformedBoundingBox->enlargeToIncludePoint(&corners[i]);
    }

    const float xDelta = transformedBoundingBox->dx() * Numeric::EPSILON_FLOAT;
    const float yDelta = transformedBoundingBox->dy() * Numeric::EPSILON_FLOAT;
    const float zDelta = transformedBoundingBox->dz() * Numeric::EPSILON_FLOAT;
    Vector3D minPoint = transformedBoundingBox->minPoint();
    Vector3D maxPoint = transformedBoundingBox->maxPoint();
    minPoint.x -= xDelta;
    minPoint.y -= yDelta;
    minPoint.z -= zDelta;
    maxPoint.x += xDelta;
    maxPoint.y += yDelta;
    maxPoint.z += zDelta;

    RpkAxisAlignedBoundingBox expandedBoundingBox;
    expandedBoundingBox.enlargeToIncludePoint(&minPoint);
    expandedBoundingBox.enlargeToIncludePoint(&maxPoint);
    transformedBoundingBox->copyFrom(&expandedBoundingBox);
}

Matrix4x4
Camera::projectionMatrixFromBoundingBox(const RpkAxisAlignedBoundingBox &boundingBox) {
    return Matrix4x4::createOrthogonalViewMatrix(
        boundingBox.minX(),
        boundingBox.maxX(),
        boundingBox.minY(),
        boundingBox.maxY(),
        -boundingBox.maxZ(),
        -boundingBox.minZ()
    );
}
