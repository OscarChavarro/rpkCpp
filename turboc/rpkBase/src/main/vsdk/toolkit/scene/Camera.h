#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CAMERA__
#define __CAMERA__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/scene/Plane.h"
#include "vsdk/toolkit/skin/AxisAlignedBoundingBox.h"

class Camera {
  public:
    #define NUMBER_OF_VIEW_PLANES 4

    Vector3Df eyePosition; // Virtual camera position in 3D space
    Vector3Df lookPosition; // Focus point of camera
    Vector3Df upDirection; // Direction pointing upward
    float viewDistance; // Distance from eye point to focus point
    float fieldOfVision; // Field of view, horizontal and vertical, in degrees
    float horizontalFov;
    float verticalFov;
    float near; // Far clipping plane distance
    float far; // Far clipping plane distance
    int xSize; // Horizontal resolution
    int ySize; // Vertical resolution
    Vector3Df X; // Eye coordinate system: X = right
    Vector3Df Y; // Eye coordinate system: Y = down
    Vector3Df Z; // Eye coordinate system: Z = viewing direction
    ColorRgbMutable background; // Window background color
    int changed; // True when camera position has been updated
    float pixelWidth;
    float pixelHeight;
    float pixelWidthTangent;
    float pixelHeightTangent;
    Plane viewPlanes[NUMBER_OF_VIEW_PLANES]; // Clipping planes

    Camera();

    void
    set(
        const Vector3Df *inEyePosition,
        const Vector3Df *inLoopPosition,
        const Vector3Df *inUpDirection,
        float inFieldOfVision,
        int inXSize,
        int inYSize,
        const ColorRgbMutable *inBackground);

    void setEyePosition(float x, float y, float z);
    void setLookPosition(float x, float y, float z);
    void setUpDirection(float x, float y, float z);
    void setFieldOfView(float fieldOfView);
    static void transformBoundingBox(const BoundingBox &sourceBoundingBox, const Matrix4x4f &transform, BoundingBox *transformedBoundingBox);
    static Matrix4x4f projectionMatrixFromBoundingBox(const BoundingBox &boundingBox);
    static Matrix4x4f lookAtMatrix(const Vector3Df &eye, const Vector3Df &centre, const Vector3Df &up);
    static Matrix4x4f perspectiveMatrix(float fieldOfViewInRadians, float aspect, float near, float far);

  private:
    void computeClippingPlanes();
    Camera *complete();
};

#endif
