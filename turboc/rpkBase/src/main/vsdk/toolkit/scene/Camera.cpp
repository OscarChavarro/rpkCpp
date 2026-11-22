#include "java/lang/Float.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/skin/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/scene/Camera.h"

Camera::Camera(): background() {
    eyePosition = Vector3Df();
    lookPosition = Vector3Df();
    upDirection = Vector3Df();
    viewDistance = 0.0f;
    fieldOfVision = 0.0f;
    horizontalFov = 0.0f;
    verticalFov = 0.0f;
    near = 0.0f;
    far = 0.0f;
    xSize = 0;
    ySize = 0;
    X = Vector3Df();
    Y = Vector3Df();
    Z = Vector3Df();
    background = ColorRgbMutable();
    changed = 0;
    pixelWidth = 0.0f;
    pixelHeight = 0.0f;
    pixelWidthTangent = 0.0f;
    pixelHeightTangent = 0.0f;
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
NULL if this fails, and a pointer to the camera arg if success
*/
Camera *
Camera::complete() {
    // Compute viewing direction ==> Z axis of eye coordinate system
    Z = lookPosition.subtract(eyePosition);

    // Distance from virtual camera position to focus point
    viewDistance = Z.length();
    if ( viewDistance < VSDK::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "eye point and look-point coincide");
        return NULL;
    }
    Z = Z.multiplyByInverse(viewDistance, Numeric::EPSILON_FLOAT);

    // camera->X is a direction pointing to the right in the window
    X = Z.crossProduct(upDirection);
    const float n = X.length();
    if ( n < VSDK::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "up-direction and viewing direction coincide");
        return NULL;
    }
    X = X.multiplyByInverse(n, Numeric::EPSILON_FLOAT);

    // camera->Y is a direction pointing down in the window
    Y = Z.crossProduct(X);
    Y = Y.normalized(Numeric::EPSILON_FLOAT);

    // Compute horizontal and vertical field of view angle from the specified one
    if ( xSize < ySize ) {
        horizontalFov = fieldOfVision;
        verticalFov = ((float)(Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                  ((float)(ySize)) / ((float)(xSize))))) * 180.0f / ((float)(M_PI));
    } else {
        verticalFov = fieldOfVision;
        horizontalFov = ((float)(Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                    ((float)(xSize)) / ((float)(ySize))))) * 180.0f / ((float)(M_PI));
    }

    // Default near and far clipping plane distance, will be set to a more reasonable
    // value when setting the camera for rendering
    near = Numeric::EPSILON_FLOAT;
    far = 2.0f * viewDistance;

    // Compute some extra frequently used quantities
    pixelWidthTangent = ((float)(Math::tan(horizontalFov * M_PI / 180.0)));
    pixelHeightTangent = ((float)(Math::tan(verticalFov * M_PI / 180.0)));

    pixelWidth = 2.0f * pixelWidthTangent / ((float)(xSize));
    pixelHeight = 2.0f * pixelHeightTangent / ((float)(ySize));

    computeClippingPlanes();

    return this;
}

/**
Sets virtual camera position, focus point, up-direction, field of view
(in degrees), horizontal and vertical window resolution and window
inBackground. Returns (CAMERA *)NULL if eye point and focus point coincide or
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
background. Returns (CAMERA *)NULL if eye point and focus point coincide or
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
    const BoundingBox &sourceBoundingBox,
    const Matrix4x4f &transform,
    BoundingBox *transformedBoundingBox)
{
    if ( transformedBoundingBox == NULL ) {
        return;
    }

    Vector3Df corners[8];
    sourceBoundingBox.corners(corners);

    *transformedBoundingBox = BoundingBox();
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

    BoundingBox expandedBoundingBox;
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
        {X.x(), X.y(), X.z(), 0.0f},
        {Y.x(), Y.y(), Y.z(), 0.0f},
        {Z.x(), Z.y(), Z.z(), 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    // Translate the eye to the origin, then orient
    return Matrix4x4f(orientation).multiply(Matrix4x4f().translation(eye.multiply(-1.0f)));
}

/**
Perspective projection like gluPerspective
*/
Matrix4x4f
Camera::perspectiveMatrix(const float fieldOfViewInRadians, const float aspect, const float near, const float far) {
    const float f = 1.0f / Math::tan(fieldOfViewInRadians / 2.0f);
    const float projection[4][4] = {
        {f / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, f, 0.0f, 0.0f},
        {0.0f, 0.0f, (near + far) / (near - far), (2 * far * near) / (near - far)},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };
    return Matrix4x4f(projection);
}

Matrix4x4f
Camera::projectionMatrixFromBoundingBox(const BoundingBox &boundingBox) {
    return Matrix4x4f().orthogonalProjection(
        boundingBox.minX(),
        boundingBox.maxX(),
        boundingBox.minY(),
        boundingBox.maxY(),
        -boundingBox.maxZ(),
        -boundingBox.minZ()
    );
}
