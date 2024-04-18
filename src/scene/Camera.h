#ifndef __CAMERA__
#define __CAMERA__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "scene/Plane.h"

#define NUMBER_OF_VIEW_PLANES 4

class Camera {
  public:
    Vector3D eyePosition; // Virtual camera position in 3D space
    Vector3D lookPosition; // Focus point of camera
    Vector3D upDirection; // Direction pointing upward
    float viewDistance; // Distance from eye point to focus point
    float fieldOfVision; // Field of view, horizontal and vertical, in degrees
    float horizontalFov;
    float verticalFov;
    float near; // Far clipping plane distance
    float far; // Far clipping plane distance
    int xSize; // Horizontal resolution
    int ySize; // Vertical resolution
    Vector3D X; // Eye coordinate system: X = right
    Vector3D Y; // Eye coordinate system: Y = down
    Vector3D Z; // Eye coordinate system: Z = viewing direction
    ColorRgb background; // Window background color
    int changed; // True when camera position has been updated
    float pixelWidth;
    float pixelHeight;
    float pixelWidthTangent;
    float pixelHeightTangent;
    Plane viewPlanes[NUMBER_OF_VIEW_PLANES]; // Clipping planes

    Camera();

    void
    set(
        Vector3D *inEyePosition,
        Vector3D *inLoopPosition,
        Vector3D *inUpDirection,
        float inFieldOfVision,
        int inXSize,
        int inYSize,
        ColorRgb *inBackground);

    void setEyePosition(float x, float y, float z);
    void setLookPosition(float x, float y, float z);
    void setUpDirection(float x, float y, float z);
    void setFieldOfView(float fieldOfView);
};

#endif
