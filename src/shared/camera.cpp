#include "common/mymath.h"
#include "common/error.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "shared/camera.h"
#include "shared/options.h"
#include "shared/defaults.h"

CAMERA GLOBAL_camera_mainCamera;    /* the one and only virtual camera */
CAMERA GLOBAL_camera_alternateCamera;       /* The second one and only camera (jp) */

/* a stack of virtual camera positions, used for temporary saving the camera and
 * later restoring */
static CAMERA CamStack[MAXCAMSTACK], *CamStackPtr = CamStack;

void CameraDefaults() {
    Vector3D eyep = DEFAULT_EYEP,
            lookp = DEFAULT_LOOKP,
            updir = DEFAULT_UPDIR;
    RGB backgroundcolor = DEFAULT_BACKGROUND_COLOR;

    CameraSet(&GLOBAL_camera_mainCamera, &eyep, &lookp, &updir, DEFAULT_FOV, 600, 600,
              &backgroundcolor);

    GLOBAL_camera_alternateCamera = GLOBAL_camera_mainCamera;
}

static void _setEyepOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetEyep(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void _setLookpOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetLookp(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void _setUpdirOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetUpdir(&GLOBAL_camera_mainCamera, v->x, v->y, v->z);
}

static void _setFovOption(void *val) {
    float *v = (float *) val;
    CameraSetFov(&GLOBAL_camera_mainCamera, *v);
}

/* And once more the same for the alternate camera */

static void _setAEyepOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetEyep(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void _setALookpOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetLookp(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void _setAUpdirOption(void *val) {
    Vector3D *v = (Vector3D *) val;
    CameraSetUpdir(&GLOBAL_camera_alternateCamera, v->x, v->y, v->z);
}

static void _setAFovOption(void *val) {
    float *v = (float *) val;
    CameraSetFov(&GLOBAL_camera_alternateCamera, *v);
}

static CMDLINEOPTDESC cameraOptions[] = {
        {"-eyepoint",  4, TVECTOR, &GLOBAL_camera_mainCamera.eyep,  _setEyepOption,
                "-eyepoint  <vector>\t: viewing position"},
        {"-center",    4, TVECTOR, &GLOBAL_camera_mainCamera.lookp, _setLookpOption,
                "-center    <vector>\t: point looked at"},
        {"-updir",     3, TVECTOR, &GLOBAL_camera_mainCamera.updir, _setUpdirOption,
                "-updir     <vector>\t: direction pointing up"},
        {"-fov",       4, Tfloat,  &GLOBAL_camera_mainCamera.fov,   _setFovOption,
                "-fov       <float> \t: field of view angle"},
        {"-aeyepoint", 4, TVECTOR, &GLOBAL_camera_alternateCamera.eyep,  _setAEyepOption,
                "-aeyepoint <vector>\t: alternate viewing position"},
        {"-acenter",   4, TVECTOR, &GLOBAL_camera_alternateCamera.lookp, _setALookpOption,
                "-acenter   <vector>\t: alternate point looked at"},
        {"-aupdir",    3, TVECTOR, &GLOBAL_camera_alternateCamera.updir, _setAUpdirOption,
                "-aupdir    <vector>\t: alternate direction pointing up"},
        {"-afov",      4, Tfloat,  &GLOBAL_camera_alternateCamera.fov,   _setAFovOption,
                "-afov      <float> \t: alternate field of view angle"},
        {nullptr,         0, TYPELESS, nullptr, DEFAULT_ACTION,
                nullptr}
};

void ParseCameraOptions(int *argc, char **argv) {
    ParseOptions(cameraOptions, argc, argv);
}

/* Sets virtual camera position, focus point, up-direction, field of view
 * (in degrees), horizontal and vertical window resolution and window
 * background. Returns (CAMERA *)nullptr if eyepoint and focus point coincide or
 * viewing direction is equal to the up-direction. */
CAMERA *CameraSet(CAMERA *Camera, Vector3D *eyep, Vector3D *lookp, Vector3D *updir,
                  float fov, int hres, int vres, RGB *background) {
    Camera->eyep = *eyep;
    Camera->lookp = *lookp;
    Camera->updir = *updir;
    Camera->fov = fov;
    Camera->hres = hres;
    Camera->vres = vres;
    Camera->background = *background;
    Camera->changed = true;

    CameraComplete(Camera);

    return Camera;
}

void CameraComputeClippingPlanes(CAMERA *Camera) {
    float x = Camera->tanhfov * Camera->viewdist;    /* half the width of the virtual screen in 3D space */
    float y = Camera->tanvfov * Camera->viewdist;    /* half the height of the virtual screen */
    Vector3D vscrn[4];
    int i;

    VECTORCOMB3(Camera->lookp, x, Camera->X, -y, Camera->Y, vscrn[0]); /* upper right corner: Y axis positions down! */
    VECTORCOMB3(Camera->lookp, x, Camera->X, y, Camera->Y, vscrn[1]); /* lower right */
    VECTORCOMB3(Camera->lookp, -x, Camera->X, y, Camera->Y, vscrn[2]); /* lower left */
    VECTORCOMB3(Camera->lookp, -x, Camera->X, -y, Camera->Y, vscrn[3]); /* upper left */

    for ( i = 0; i < 4; i++ ) {
        VECTORTRIPLECROSSPRODUCT(vscrn[(i + 1) % 4], Camera->eyep, vscrn[i], Camera->viewplane[i].norm);
        VECTORNORMALIZE(Camera->viewplane[i].norm);
        Camera->viewplane[i].d = -VECTORDOTPRODUCT(Camera->viewplane[i].norm, Camera->eyep);
    }
}

CAMERA *CameraComplete(CAMERA *Camera) {
    float n;

    /* compute viewing direction ==> Z axis of eye coordinate system */
    VECTORSUBTRACT(Camera->lookp, Camera->eyep, Camera->Z);

    /* distance from virtual camera position to focus point */
    Camera->viewdist = VECTORNORM(Camera->Z);
    if ( Camera->viewdist < EPSILON ) {
        Error("SetCamera", "eyepoint and look-point coincide");
        return nullptr;
    }
    VECTORSCALEINVERSE(Camera->viewdist, Camera->Z, Camera->Z);

    /* GLOBAL_camera_mainCamera->X is a direction pointing to the right in the window */
    VECTORCROSSPRODUCT(Camera->Z, Camera->updir, Camera->X);
    n = VECTORNORM(Camera->X);
    if ( n < EPSILON ) {
        Error("SetCamera", "up-direction and viewing direction coincide");
        return nullptr;
    }
    VECTORSCALEINVERSE(n, Camera->X, Camera->X);

    /* GLOBAL_camera_mainCamera->Y is a direction pointing down in the window */
    VECTORCROSSPRODUCT(Camera->Z, Camera->X, Camera->Y);
    VECTORNORMALIZE(Camera->Y);

    /* compute horizontal and vertical field of view angle from the specified one */
    if ( Camera->hres < Camera->vres ) {
        Camera->hfov = Camera->fov;
        Camera->vfov = atan(tan(Camera->fov * M_PI / 180.) *
                            (float) Camera->vres / (float) Camera->hres) * 180. / M_PI;
    } else {
        Camera->vfov = Camera->fov;
        Camera->hfov = atan(tan(Camera->fov * M_PI / 180.) *
                            (float) Camera->hres / (float) Camera->vres) * 180. / M_PI;
    }

    /* default near and far clipping plane distance, will be set to a more reasonable
    * value when setting the camera for rendering. */
    Camera->near = EPSILON;
    Camera->far = 2. * Camera->viewdist;

    /* Compute some extra frequently used quantities */
    Camera->tanhfov = tan(Camera->hfov * M_PI / 180.0);
    Camera->tanvfov = tan(Camera->vfov * M_PI / 180.0);

    Camera->pixh = 2.0 * Camera->tanhfov / (float) (Camera->hres);
    Camera->pixv = 2.0 * Camera->tanvfov / (float) (Camera->vres);

    /*
    GLOBAL_camera_mainCamera->pixh = 2.0 * GLOBAL_camera_mainCamera->tanhfov / (float)(GLOBAL_camera_mainCamera->hres - 1);
    GLOBAL_camera_mainCamera->pixv = 2.0 * GLOBAL_camera_mainCamera->tanvfov / (float)(GLOBAL_camera_mainCamera->vres - 1);
    */

    CameraComputeClippingPlanes(Camera);

    return Camera;
}

/* only sets virtual camera position in 3D space */
CAMERA *CameraSetEyep(CAMERA *cam, float x, float y, float z) {
    Vector3D neweyep;
    VECTORSET(neweyep, x, y, z);
    return CameraSet(cam, &neweyep, &cam->lookp, &cam->updir,
                     cam->fov, cam->hres, cam->vres, &cam->background);
}

CAMERA *CameraSetLookp(CAMERA *cam, float x, float y, float z) {
    Vector3D newlookp;
    VECTORSET(newlookp, x, y, z);
    return CameraSet(cam, &cam->eyep, &newlookp, &cam->updir,
                     cam->fov, cam->hres, cam->vres, &cam->background);
}

CAMERA *CameraSetUpdir(CAMERA *cam, float x, float y, float z) {
    Vector3D newupdir;
    VECTORSET(newupdir, x, y, z);
    return CameraSet(cam, &cam->eyep, &cam->lookp, &newupdir,
                     cam->fov, cam->hres, cam->vres, &cam->background);
}

CAMERA *CameraSetFov(CAMERA *cam, float fov) {
    return CameraSet(cam, &cam->eyep, &cam->lookp, &cam->updir,
                     fov, cam->hres, cam->vres, &cam->background);
}

/* returns pointer to the next saved camera. If previous==nullptr, the first saved
 * camera is returned. In subsequent calls, the previous camera returned
 * by this function should be passed as the parameter. If all saved cameras
 * have been iterated over, nullptr is returned. */
CAMERA *NextSavedCamera(CAMERA *previous) {
    CAMERA *cam = previous ? previous : CamStackPtr;
    cam--;
    return (cam < CamStack) ? (CAMERA *) nullptr : cam;
}
