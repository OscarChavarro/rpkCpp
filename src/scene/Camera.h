#ifndef __CAMERA__
#define __CAMERA__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"

class CameraClippingPlane {
  public:
    Vector3D normal;
    float d;

    CameraClippingPlane(): normal(), d{} {}
};

#define NUMBER_OF_VIEW_PLANES 4

class Camera {
  public:
    Vector3D eyePosition; // Virtual camera position in 3D space
    Vector3D lookPosition; // Focus point of camera
    Vector3D upDirection; // Direction pointing upward
    float viewDistance; // Distance from eye point to focus point
    float fov; // Field of view, horizontal and vertical, in degrees
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
    CameraClippingPlane viewPlane[NUMBER_OF_VIEW_PLANES];

    Camera();
};

extern Camera GLOBAL_camera_mainCamera;

extern Camera *
cameraSet(
    Camera *camera,
    Vector3D *eyePosition,
    Vector3D *loopPosition,
    Vector3D *upDirection,
    float fov,
    int xSize,
    int ySize,
    ColorRgb *background);

extern Camera *cameraComplete(Camera *camera);
extern Camera *cameraSetFieldOfView(Camera *camera, float fov);
extern Camera *cameraSetUpDirection(Camera *camera, float x, float y, float z);
extern Camera *cameraSetLookPosition(Camera *camera, float x, float y, float z);
extern Camera *cameraSetEyePosition(Camera *camera, float x, float y, float z);
extern Camera *nextSavedCamera(Camera *previous);
extern void parseCameraOptions(int *argc, char **argv);
extern void cameraDefaults();

#endif
