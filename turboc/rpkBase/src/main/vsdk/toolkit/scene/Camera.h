#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CAMERA__
#define __CAMERA__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/scene/Plane.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"

class Camera {
  public:
    #define NUMBER_OF_VIEW_PLANES 4

    Vector3Dd eyePosition; // Virtual camera position in 3D space
    Vector3Dd lookPosition; // Focus point of camera
    Vector3Dd upDirection; // Direction pointing upward
    double viewDistance; // Distance from eye point to focus point
    double fieldOfVision; // Field of view, horizontal and vertical, in degrees
    double horizontalFov;
    double verticalFov;
    double near; // Far clipping plane distance
    double far; // Far clipping plane distance
    int xSize; // Horizontal resolution
    int ySize; // Vertical resolution
    Vector3Dd X; // Eye coordinate system: X = right
    Vector3Dd Y; // Eye coordinate system: Y = down
    Vector3Dd Z; // Eye coordinate system: Z = viewing direction
    ColorRgbMutable background; // Window background color
    int changed; // True when camera position has been updated
    double pixelWidth;
    double pixelHeight;
    double pixelWidthTangent;
    double pixelHeightTangent;
    Plane viewPlanes[NUMBER_OF_VIEW_PLANES]; // Clipping planes

    Camera();

    void
    set(
        const Vector3Dd *inEyePosition,
        const Vector3Dd *inLoopPosition,
        const Vector3Dd *inUpDirection,
        double inFieldOfVision,
        int inXSize,
        int inYSize,
        const ColorRgbMutable *inBackground);

    void setEyePosition(double x, double y, double z);
    void setLookPosition(double x, double y, double z);
    void setUpDirection(double x, double y, double z);
    void setFieldOfView(double fieldOfView);
    static void transformBoundingBox(const RpkAxisAlignedBoundingBox &sourceBoundingBox, const Matrix4x4d &transform, RpkAxisAlignedBoundingBox *transformedBoundingBox);
    static Matrix4x4d projectionMatrixFromBoundingBox(const RpkAxisAlignedBoundingBox &boundingBox);
    static Matrix4x4d lookAtMatrix(const Vector3Dd &eye, const Vector3Dd &centre, const Vector3Dd &up);
    static Matrix4x4d perspectiveMatrix(double fieldOfViewInRadians, double aspect, double near, double far);

  private:
    void computeClippingPlanes();
    Camera *complete();
};

#endif
