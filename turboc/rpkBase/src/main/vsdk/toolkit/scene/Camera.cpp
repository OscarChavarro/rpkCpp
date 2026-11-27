#include "java/lang/Double.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/scene/Camera.h"

Camera::Camera(): background() {
    eyePosition = Vector3Dd();
    lookPosition = Vector3Dd();
    upDirection = Vector3Dd();
    viewDistance = 0.0;
    fieldOfVision = 0.0;
    horizontalFov = 0.0;
    verticalFov = 0.0;
    near = 0.0;
    far = 0.0;
    xSize = 0;
    ySize = 0;
    X = Vector3Dd();
    Y = Vector3Dd();
    Z = Vector3Dd();
    background = ColorRgbMutable();
    changed = 0;
    pixelWidth = 0.0;
    pixelHeight = 0.0;
    pixelWidthTangent = 0.0;
    pixelHeightTangent = 0.0;
}

void
Camera::computeClippingPlanes() {
    double x = pixelWidthTangent * viewDistance; // Half the width of the virtual screen in 3D space
    double y = pixelHeightTangent * viewDistance; // Half the height of the virtual screen
    Vector3Dd vScreen[4];

    vScreen[0] = lookPosition.add(X.multiply(x)).add(Y.multiply(-y)); // Upper right corner: Y axis positions down!
    vScreen[1] = lookPosition.add(X.multiply(x)).add(Y.multiply(y)); // Lower right
    vScreen[2] = lookPosition.add(X.multiply(-x)).add(Y.multiply(y)); // Lower left
    vScreen[3] = lookPosition.add(X.multiply(-x)).add(Y.multiply(-y)); // Upper left

    for ( int i = 0; i < 4; i++ ) {
        viewPlanes[i].normal = vScreen[i].subtract(eyePosition).crossProduct((vScreen[(i + 1) % 4]).subtract(eyePosition));
        viewPlanes[i].normal = viewPlanes[i].normal.normalized(Numeric::EPSILON);
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
    Z = Z.multiplyByInverse(viewDistance, Numeric::EPSILON);

    // camera->X is a direction pointing to the right in the window
    X = Z.crossProduct(upDirection);
    const double n = X.length();
    if ( n < VSDK::EPSILON ) {
        Logger::reportMessage("Camera", Logger::ERROR, "SetCamera", "up-direction and viewing direction coincide");
        return NULL;
    }
    X = X.multiplyByInverse(n, Numeric::EPSILON);

    // camera->Y is a direction pointing down in the window
    Y = Z.crossProduct(X);
    Y = Y.normalized(Numeric::EPSILON);

    // Compute horizontal and vertical field of view angle from the specified one
    if ( xSize < ySize ) {
        horizontalFov = fieldOfVision;
        verticalFov = ((double)(Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                  ((double)(ySize)) / ((double)(xSize))))) * 180.0 / ((double)(M_PI));
    } else {
        verticalFov = fieldOfVision;
        horizontalFov = ((double)(Math::atan(tan(fieldOfVision * M_PI / 180.0) *
                                                                    ((double)(xSize)) / ((double)(ySize))))) * 180.0 / ((double)(M_PI));
    }

    // Default near and far clipping plane distance, will be set to a more reasonable
    // value when setting the camera for rendering
    near = Numeric::EPSILON;
    far = 2.0 * viewDistance;

    // Compute some extra frequently used quantities
    pixelWidthTangent = ((double)(Math::tan(horizontalFov * M_PI / 180.0)));
    pixelHeightTangent = ((double)(Math::tan(verticalFov * M_PI / 180.0)));

    pixelWidth = 2.0 * pixelWidthTangent / ((double)(xSize));
    pixelHeight = 2.0 * pixelHeightTangent / ((double)(ySize));

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
    const Vector3Dd *inEyePosition,
    const Vector3Dd *inLoopPosition,
    const Vector3Dd *inUpDirection,
    const double inFieldOfVision,
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
Camera::setEyePosition(double x, double y, double z) {
    Vector3Dd newEyePosition;
    newEyePosition = Vector3Dd(x, y, z);
    set(&newEyePosition, &lookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setLookPosition(double x, double y, double z) {
    Vector3Dd newLookPosition;
    newLookPosition = Vector3Dd(x, y, z);
    set(&eyePosition, &newLookPosition, &upDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setUpDirection(double x, double y, double z) {
    Vector3Dd newUpDirection;
    newUpDirection = Vector3Dd(x, y, z);
    set(&eyePosition, &lookPosition, &newUpDirection, fieldOfVision, xSize, ySize, &background);
}

void
Camera::setFieldOfView(double fieldOfView) {
    set(&eyePosition, &lookPosition, &upDirection, fieldOfView, xSize, ySize, &background);
}

void
Camera::transformBoundingBox(
    const RpkAxisAlignedBoundingBox &sourceBoundingBox,
    const Matrix4x4d &transform,
    RpkAxisAlignedBoundingBox *transformedBoundingBox)
{
    if ( transformedBoundingBox == NULL ) {
        return;
    }

    Vector3Dd corners[8];
    sourceBoundingBox.corners(corners);

    *transformedBoundingBox = RpkAxisAlignedBoundingBox();
    for ( int i = 0; i < 8; i++ ) {
        // Rotation part only (the 3x3 block), as RenderPark always did
        corners[i] = transform.withoutTranslation().multiply(corners[i]);
        transformedBoundingBox->enlargeToIncludePoint(&corners[i]);
    }

    const double xDelta = transformedBoundingBox->dx() * Numeric::EPSILON;
    const double yDelta = transformedBoundingBox->dy() * Numeric::EPSILON;
    const double zDelta = transformedBoundingBox->dz() * Numeric::EPSILON;
    Vector3Dd minPoint = transformedBoundingBox->minPoint();
    Vector3Dd maxPoint = transformedBoundingBox->maxPoint();
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
Matrix4x4d
Camera::lookAtMatrix(const Vector3Dd &eye, const Vector3Dd &centre, const Vector3Dd &up) {
    const Vector3Dd Z = eye.subtract(centre).normalized(Numeric::EPSILON); // Z positions towards viewer
    const Vector3Dd X = up.crossProduct(Z).normalized(Numeric::EPSILON); // X positions right
    const Vector3Dd Y = Z.crossProduct(X); // Y positions up
    const double orientation[4][4] = {
        {X.x(), X.y(), X.z(), 0.0},
        {Y.x(), Y.y(), Y.z(), 0.0},
        {Z.x(), Z.y(), Z.z(), 0.0},
        {0.0, 0.0, 0.0, 1.0}
    };
    // Translate the eye to the origin, then orient
    return Matrix4x4d(orientation).multiply(Matrix4x4d().translation(eye.multiply(-1.0)));
}

/**
Perspective projection like gluPerspective
*/
Matrix4x4d
Camera::perspectiveMatrix(const double fieldOfViewInRadians, const double aspect, const double near, const double far) {
    const double f = 1.0 / Math::tan(fieldOfViewInRadians / 2.0);
    const double projection[4][4] = {
        {f / aspect, 0.0, 0.0, 0.0},
        {0.0, f, 0.0, 0.0},
        {0.0, 0.0, (near + far) / (near - far), (2 * far * near) / (near - far)},
        {0.0, 0.0, -1.0, 0.0}
    };
    return Matrix4x4d(projection);
}

Matrix4x4d
Camera::projectionMatrixFromBoundingBox(const RpkAxisAlignedBoundingBox &boundingBox) {
    return Matrix4x4d().orthogonalProjection(
        boundingBox.minX(),
        boundingBox.maxX(),
        boundingBox.minY(),
        boundingBox.maxY(),
        -boundingBox.maxZ(),
        -boundingBox.minZ()
    );
}
