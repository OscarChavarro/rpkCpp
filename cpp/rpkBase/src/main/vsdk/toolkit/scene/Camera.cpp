#include "java/lang/Float.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/scene/Camera.h"

Camera::Camera(): background() {
    eyePosition = Vector3Df{};
    lookPosition = Vector3Df{};
    upDirection = Vector3Df{};
    viewDistance = 0.0F;
    fieldOfVision = 0.0F;
    horizontalFov = 0.0F;
    verticalFov = 0.0F;
    near = 0.0F;
    far = 0.0F;
    xSize = 0;
    ySize = 0;
    X = Vector3Df{};
    Y = Vector3Df{};
    Z = Vector3Df{};
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
    Vector3Df minimum;
    Vector3Df maximum;
    Vector3Df d;

    minimum = Vector3Df(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
    maximum = Vector3Df(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());

    far = -java::Float::MAX_VALUE;
    near = java::Float::MAX_VALUE;
    for ( int i = 0; i <= 1; i++ ) {
        for ( int j = 0; j <= 1; j++ ) {
            for ( int k = 0; k <= 1; k++ ) {
                d = Vector3Df(i ? maximum.x() : minimum.x(), j ? maximum.y() : minimum.y(), k ? maximum.z() : minimum.z());

                d = d.subtract(eyePosition);
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
    if ( far < VSDK::EPSILON ) {
        far = viewDistance;
    }
    if ( near < VSDK::EPSILON ) {
        near = viewDistance / 100.0F;
    }
}

void
Camera::computeClippingPlanes() {
    float x = pixelWidthTangent * viewDistance; // Half the width of the virtual screen in 3D space
    float y = pixelHeightTangent * viewDistance; // Half the height of the virtual screen
    Vector3Df vScreen[4];

    vScreen[0] = lookPosition.add(X.multiply(x)).add(Y.multiply(-y)); // Upper right corner: Y axis positions down!
    vScreen[1] = lookPosition.add(X.multiply(x)).add(Y.multiply(y)); // Lower right
    vScreen[2] = lookPosition.add(X.multiply(-x)).add(Y.multiply(y)); // Lower left
    vScreen[3] = lookPosition.add(X.multiply(-x)).add(Y.multiply(-y)); // Upper left

    for ( int i = 0; i < 4; i++ ) {
        viewPlanes[i].normal = vScreen[i].subtract(eyePosition).crossProduct((vScreen[(i + 1) % 4]).subtract(eyePosition));
        viewPlanes[i].normal = viewPlanes[i].normal.normalized(Numeric::EPSILON_FLOAT);
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
    Z = lookPosition.subtract(eyePosition);

    // Distance from virtual camera position to focus point
    viewDistance = Z.length();
    if ( viewDistance < VSDK::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "eye point and look-point coincide");
        return nullptr;
    }
    Z = Z.multiplyByInverse(viewDistance, Numeric::EPSILON_FLOAT);

    // camera->X is a direction pointing to the right in the window
    X = Z.crossProduct(upDirection);
    const float n = X.length();
    if ( n < VSDK::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "up-direction and viewing direction coincide");
        return nullptr;
    }
    X = X.multiplyByInverse(n, Numeric::EPSILON_FLOAT);

    // camera->Y is a direction pointing down in the window
    Y = Z.crossProduct(X);
    Y = Y.normalized(Numeric::EPSILON_FLOAT);

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
    const Vector3Df *inEyePosition,
    const Vector3Df *inLoopPosition,
    const Vector3Df *inUpDirection,
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
    Vector3Df newEyePosition;
    newEyePosition = Vector3Df(x, y, z);
    set(&newEyePosition, &lookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setLookPosition(float x, float y, float z) {
    Vector3Df newLookPosition;
    newLookPosition = Vector3Df(x, y, z);
    set(&eyePosition, &newLookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setUpDirection(float x, float y, float z) {
    Vector3Df newUpDirection;
    newUpDirection = Vector3Df(x, y, z);
    set(&eyePosition, &lookPosition, &newUpDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setFieldOfView(float fieldOfView) {
    set(&eyePosition, &lookPosition, &upDirection, fieldOfView, xSize, ySize, &background);
}

void
Camera::transformBoundingBox(
    const RpkAxisAlignedBoundingBox &sourceBoundingBox,
    const Matrix4x4f &transform,
    RpkAxisAlignedBoundingBox *transformedBoundingBox)
{
    if ( transformedBoundingBox == nullptr ) {
        return;
    }

    Vector3Df corners[8];
    sourceBoundingBox.corners(corners);

    *transformedBoundingBox = RpkAxisAlignedBoundingBox{};
    for ( int i = 0; i < 8; i++ ) {
        // Rotation part only (the 3x3 block), as RenderPark always did
        corners[i] = transform.withoutTranslation().multiply(corners[i]);
        transformedBoundingBox->enlargeToIncludePoint(&corners[i]);
    }

    const float xDelta = transformedBoundingBox->dx() * Numeric::EPSILON_FLOAT;
    const float yDelta = transformedBoundingBox->dy() * Numeric::EPSILON_FLOAT;
    const float zDelta = transformedBoundingBox->dz() * Numeric::EPSILON_FLOAT;
    Vector3Df minPoint = transformedBoundingBox->minPoint();
    Vector3Df maxPoint = transformedBoundingBox->maxPoint();
    minPoint = minPoint.withX(minPoint.x() - (xDelta));
    minPoint = minPoint.withY(minPoint.y() - (yDelta));
    minPoint = minPoint.withZ(minPoint.z() - (zDelta));
    maxPoint = maxPoint.withX(maxPoint.x() + (xDelta));
    maxPoint = maxPoint.withY(maxPoint.y() + (yDelta));
    maxPoint = maxPoint.withZ(maxPoint.z() + (zDelta));

    RpkAxisAlignedBoundingBox expandedBoundingBox;
    expandedBoundingBox.enlargeToIncludePoint(&minPoint);
    expandedBoundingBox.enlargeToIncludePoint(&maxPoint);
    transformedBoundingBox->copyFrom(&expandedBoundingBox);
}

/**
Transforms the eye point to the origin and rotates so that the centre lies
on the negative Z axis and the up direction on the positive Y axis (Y up,
X right, Z towards the viewer)
*/
Matrix4x4f
Camera::lookAtMatrix(const Vector3Df &eye, const Vector3Df &centre, const Vector3Df &up) {
    const Vector3Df Z = eye.subtract(centre).normalized(Numeric::EPSILON_FLOAT); // Z positions towards viewer
    const Vector3Df X = up.crossProduct(Z).normalized(Numeric::EPSILON_FLOAT); // X positions right
    const Vector3Df Y = Z.crossProduct(X); // Y positions up
    const float orientation[4][4] = {
        {X.x(), X.y(), X.z(), 0.0F},
        {Y.x(), Y.y(), Y.z(), 0.0F},
        {Z.x(), Z.y(), Z.z(), 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F}
    };
    // Translate the eye to the origin, then orient
    return Matrix4x4f(orientation).multiply(Matrix4x4f().translation(eye.multiply(-1.0F)));
}

/**
Perspective projection like gluPerspective
*/
Matrix4x4f
Camera::perspectiveMatrix(const float fieldOfViewInRadians, const float aspect, const float near, const float far) {
    const float f = 1.0F / java::Math::tan(fieldOfViewInRadians / 2.0F);
    const float projection[4][4] = {
        {f / aspect, 0.0F, 0.0F, 0.0F},
        {0.0F, f, 0.0F, 0.0F},
        {0.0F, 0.0F, (near + far) / (near - far), (2 * far * near) / (near - far)},
        {0.0F, 0.0F, -1.0F, 0.0F}
    };
    return Matrix4x4f(projection);
}

Matrix4x4f
Camera::projectionMatrixFromBoundingBox(const RpkAxisAlignedBoundingBox &boundingBox) {
    return Matrix4x4f().orthogonalProjection(
        boundingBox.minX(),
        boundingBox.maxX(),
        boundingBox.minY(),
        boundingBox.maxY(),
        -boundingBox.maxZ(),
        -boundingBox.minZ()
    );
}
