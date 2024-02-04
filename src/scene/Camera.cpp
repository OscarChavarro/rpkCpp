#include "common/error.h"
#include "common/options.h"
#include "scene/Camera.h"

// Default virtual camera
#define DEFAULT_EYEP {10.0, 0.0, 0.0}
#define DEFAULT_LOOKP { 0.0, 0.0, 0.0}
#define DEFAULT_UPDIR { 0.0, 0.0, 1.0}
#define DEFAULT_FOV 22.5
#define DEFAULT_BACKGROUND_COLOR {0.0, 0.0, 0.0}

// Camera position etc. can be saved on a stack of size MAXIMUM_CAMERA_STACK
#define MAXIMUM_CAMERA_STACK 20

Camera GLOBAL_camera_mainCamera; // The main virtual camera
Camera GLOBAL_camera_alternateCamera; // The second camera

// A stack of virtual camera positions, used for temporary saving the camera and
// later restoring
static Camera globalCameraStack[MAXIMUM_CAMERA_STACK];
static Camera *globalCameraStackPtr = globalCameraStack;

Camera::Camera(): background() {
    eyePosition = Vector3D{};
    lookPosition = Vector3D{};
    upDirection = Vector3D{};
    viewDistance = 0.0f;
    fov = 0.0f;
    horizontalFov = 0.0f;
    verticalFov = 0.0f;
    near = 0.0f;
    far = 0.0f;
    xSize = 0;
    ySize = 0;
    X = Vector3D{};
    Y = Vector3D{};
    Z = Vector3D{};
    background = RGB{};
    changed = 0;
    pixelWidth = 0.0f;
    pixelHeight = 0.0f;
    pixelWidthTangent = 0.0f;
    pixelHeightTangent = 0.0f;
}

void
cameraDefaults() {
    Vector3D eyePosition = DEFAULT_EYEP;
    Vector3D lookPosition = DEFAULT_LOOKP;
    Vector3D upDirection = DEFAULT_UPDIR;
    RGB backgroundColor = DEFAULT_BACKGROUND_COLOR;

    cameraSet(&GLOBAL_camera_mainCamera, &eyePosition, &lookPosition, &upDirection, DEFAULT_FOV, 600, 600,
              &backgroundColor);

    GLOBAL_camera_alternateCamera = GLOBAL_camera_mainCamera;
}

static void
cameraSetEyePositionOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetEyePosition(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void
cameraSetLookPositionOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetLookPosition(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void cameraSetUpDirectionOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetUpDirection(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void cameraSetFieldOfViewOption(void *val) {
    float *v = (float *) val;
    cameraSetFieldOfView(&GLOBAL_camera_mainCamera, *v);
}

static void cameraSetAlternativeEyePositionOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetEyePosition(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void setAlternativeLookUpOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetLookPosition(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void cameraSetAlternativeUpDirectionOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    cameraSetUpDirection(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void cameraSetAlternativeFieldOfVisionOption(void *val) {
    float *v = (float *) val;
    cameraSetFieldOfView(&GLOBAL_camera_alternateCamera, *v);
}

static CommandLineOptionDescription globalCameraOptions[] = {
        {"-eyepoint",  4, TVECTOR, &GLOBAL_camera_mainCamera.eyePosition,      cameraSetEyePositionOption,
    "-eyepoint  <vector>\t: viewing position"},
        {"-center",    4, TVECTOR, &GLOBAL_camera_mainCamera.lookPosition,      cameraSetLookPositionOption,
    "-center    <vector>\t: point looked at"},
        {"-updir",     3, TVECTOR, &GLOBAL_camera_mainCamera.upDirection,      cameraSetUpDirectionOption,
    "-updir     <vector>\t: direction pointing up"},
        {"-fov",       4, Tfloat,  &GLOBAL_camera_mainCamera.fov, cameraSetFieldOfViewOption,
    "-fov       <float> \t: field of view angle"},
        {"-aeyepoint", 4, TVECTOR, &GLOBAL_camera_alternateCamera.eyePosition, cameraSetAlternativeEyePositionOption,
    "-aeyepoint <vector>\t: alternate viewing position"},
        {"-acenter",   4, TVECTOR, &GLOBAL_camera_alternateCamera.lookPosition, setAlternativeLookUpOption,
    "-acenter   <vector>\t: alternate point looked at"},
        {"-aupdir",    3, TVECTOR, &GLOBAL_camera_alternateCamera.upDirection, cameraSetAlternativeUpDirectionOption,
    "-aupdir    <vector>\t: alternate direction pointing up"},
        {"-afov",      4, Tfloat,  &GLOBAL_camera_alternateCamera.fov, cameraSetAlternativeFieldOfVisionOption,
    "-afov      <float> \t: alternate field of view angle"},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void parseCameraOptions(int *argc, char **argv) {
    parseOptions(globalCameraOptions, argc, argv);
}

/**
Sets virtual camera position, focus point, up-direction, field of view
(in degrees), horizontal and vertical window resolution and window
background. Returns (CAMERA *)nullptr if eye point and focus point coincide or
viewing direction is equal to the up-direction
*/
Camera *
cameraSet(Camera *camera, Vector3D *eyePosition, Vector3D *loopPosition, Vector3D *upDirection,
          float fov, int xSize, int ySize, RGB *background) {
    camera->eyePosition = *eyePosition;
    camera->lookPosition = *loopPosition;
    camera->upDirection = *upDirection;
    camera->fov = fov;
    camera->xSize = xSize;
    camera->ySize = ySize;
    camera->background = *background;
    camera->changed = true;

    cameraComplete(camera);

    return camera;
}

void
cameraComputeClippingPlanes(Camera *camera) {
    float x = camera->pixelWidthTangent * camera->viewDistance; // Half the width of the virtual screen in 3D space
    float y = camera->pixelHeightTangent * camera->viewDistance; // Half the height of the virtual screen
    Vector3D vscrn[4];
    int i;

    VECTORCOMB3(camera->lookPosition, x, camera->X, -y, camera->Y, vscrn[0]); // Upper right corner: Y axis positions down!
    VECTORCOMB3(camera->lookPosition, x, camera->X, y, camera->Y, vscrn[1]); // Lower right
    VECTORCOMB3(camera->lookPosition, -x, camera->X, y, camera->Y, vscrn[2]); // Lower left
    VECTORCOMB3(camera->lookPosition, -x, camera->X, -y, camera->Y, vscrn[3]); // Upper left

    for ( i = 0; i < 4; i++ ) {
        VECTORTRIPLECROSSPRODUCT(vscrn[(i + 1) % 4], camera->eyePosition, vscrn[i], camera->viewPlane[i].normal);
        VECTORNORMALIZE(camera->viewPlane[i].normal);
        camera->viewPlane[i].d = -VECTORDOTPRODUCT(camera->viewPlane[i].normal, camera->eyePosition);
    }
}

/**
Computes camera coordinate system and horizontal and vertical fov depending
on filled in fov value and aspect ratio of the view window. Returns
nullptr if this fails, and a pointer to the camera arg if success
*/
Camera *
cameraComplete(Camera *camera) {
    float n;

    // Compute viewing direction ==> Z axis of eye coordinate system
    VECTORSUBTRACT(camera->lookPosition, camera->eyePosition, camera->Z);

    // Distance from virtual camera position to focus point
    camera->viewDistance = VECTORNORM(camera->Z);
    if ( camera->viewDistance < EPSILON ) {
        logError("SetCamera", "eye point and look-point coincide");
        return nullptr;
    }
    VECTORSCALEINVERSE(camera->viewDistance, camera->Z, camera->Z);

    // GLOBAL_camera_mainCamera->X is a direction pointing to the right in the window
    VECTORCROSSPRODUCT(camera->Z, camera->upDirection, camera->X);
    n = VECTORNORM(camera->X);
    if ( n < EPSILON ) {
        logError("SetCamera", "up-direction and viewing direction coincide");
        return nullptr;
    }
    VECTORSCALEINVERSE(n, camera->X, camera->X);

    // GLOBAL_camera_mainCamera->Y is a direction pointing down in the window
    VECTORCROSSPRODUCT(camera->Z, camera->X, camera->Y);
    VECTORNORMALIZE(camera->Y);

    // Compute horizontal and vertical field of view angle from the specified one
    if ( camera->xSize < camera->ySize ) {
        camera->horizontalFov = camera->fov;
        camera->verticalFov = (float)std::atan(tan(camera->fov * M_PI / 180.0) *
                                               (float)camera->ySize / (float) camera->xSize) * 180.0f / (float)M_PI;
    } else {
        camera->verticalFov = camera->fov;
        camera->horizontalFov = (float)std::atan(tan(camera->fov * M_PI / 180.0) *
                                                 (float)camera->xSize / (float)camera->ySize) * 180.0f / (float)M_PI;
    }

    // Default near and far clipping plane distance, will be set to a more reasonable
    // value when setting the camera for rendering
    camera->near = EPSILON;
    camera->far = 2.0f * camera->viewDistance;

    // Compute some extra frequently used quantities
    camera->pixelWidthTangent = (float)std::tan(camera->horizontalFov * M_PI / 180.0);
    camera->pixelHeightTangent = (float)std::tan(camera->verticalFov * M_PI / 180.0);

    camera->pixelWidth = 2.0f * camera->pixelWidthTangent / (float) (camera->xSize);
    camera->pixelHeight = 2.0f * camera->pixelHeightTangent / (float) (camera->ySize);

    cameraComputeClippingPlanes(camera);

    return camera;
}

/**
Only sets virtual camera position in 3D space
*/
Camera *
cameraSetEyePosition(Camera *camera, float x, float y, float z) {
    Vector3D newEyePosition;
    VECTORSET(newEyePosition, x, y, z);
    return cameraSet(camera, &newEyePosition, &camera->lookPosition, &camera->upDirection,
                     camera->fov, camera->xSize, camera->ySize, &camera->background);
}

Camera *
cameraSetLookPosition(Camera *camera, float x, float y, float z) {
    Vector3D newLookPosition;
    VECTORSET(newLookPosition, x, y, z);
    return cameraSet(camera, &camera->eyePosition, &newLookPosition, &camera->upDirection,
                     camera->fov, camera->xSize, camera->ySize, &camera->background);
}

Camera *
cameraSetUpDirection(Camera *camera, float x, float y, float z) {
    Vector3D newUpDirection;
    VECTORSET(newUpDirection, x, y, z);
    return cameraSet(camera, &camera->eyePosition, &camera->lookPosition, &newUpDirection,
                     camera->fov, camera->xSize, camera->ySize, &camera->background);
}

/**
Sets only field-of-view, up-direction, focus point, camera position
*/
Camera *
cameraSetFieldOfView(Camera *camera, float fov) {
    return cameraSet(camera, &camera->eyePosition, &camera->lookPosition, &camera->upDirection,
                     fov, camera->xSize, camera->ySize, &camera->background);
}

/**
Returns pointer to the next saved camera. If previous==nullptr, the first saved
camera is returned. In subsequent calls, the previous camera returned
by this function should be passed as the parameter. If all saved cameras
have been iterated over, nullptr is returned
*/
Camera *
nextSavedCamera(Camera *previous) {
    Camera *cam = previous ? previous : globalCameraStackPtr;
    cam--;
    return (cam < globalCameraStack) ? nullptr : cam;
}
