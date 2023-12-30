/* camera.h: virtual camera management */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "common/linealAlgebra/vectorMacros.h"
#include "material/color.h"

class CAMERA_CLIPPING_PLANE {
  public:
    Vector3D norm;
    float d;
};
#define NR_VIEW_PLANES    4

class CAMERA {
  public:
    Vector3D eyep,        /* virtual camera position in 3D space */
    lookp,        /* focus point of camera */
    updir;        /* direction pointing upward */
    float viewdist;    /* distance from eyepoint to focus point */
    float fov, hfov, vfov;/* field of view, horizontal and vertical,
			 * in degrees. */
    float near, far;    /* near and far clipping plane distance */
    int hres, vres;    /* horizontal and vertical resolution */
    Vector3D X, Y, Z;    /* eye coordinate system: X=right, Y=down, Z=viewing direction */
    RGB background;    /* window background color */
    int changed;        /* True when camera position has been updated */
    float pixh, pixv;       /* Width and height of a pixel */
    float tanhfov, tanvfov; /* Just tanges of hfov and vfov */
    CAMERA_CLIPPING_PLANE viewplane[NR_VIEW_PLANES];
};

extern CAMERA GLOBAL_camera_mainCamera;        /* The one and only virtual camera */
extern CAMERA GLOBAL_camera_alternateCamera;    /* The one and only alternate camera */

/* sets virtual camera position, focus point, up-direction, field of view
 * (in degrees), horizontal and vertical window resolution and window
 * background. Returns (CAMERA *)nullptr if eyepoint and focus point coincide or
 * viewing direction is equal to the up-direction. */
extern CAMERA *CameraSet(CAMERA *cam,
                         Vector3D *eyep, Vector3D *lookp, Vector3D *updir,
                         float fov, int hres, int vres, RGB *background);

/* Computes camera cordinate system and horizontal and vertical fov depending
 * on filled in fov value and aspect ratio of the view window. Returns
 * nullptr if this fails, and a pointer to the camera arg if succes. */
extern CAMERA *CameraComplete(CAMERA *camera);

/* sets only field-of-view, up-direction, focus point, camera position */
extern CAMERA *CameraSetFov(CAMERA *cam, float fov);

extern CAMERA *CameraSetUpdir(CAMERA *cam, float x, float y, float z);

extern CAMERA *CameraSetLookp(CAMERA *cam, float x, float y, float z);

extern CAMERA *CameraSetEyep(CAMERA *cam, float x, float y, float z);

/* camera postition etc. can be saved on a stack of size MAXCAMSTACK. */
#define MAXCAMSTACK 20

/* returns pointer to the next saved camera. If previous==nullptr, the first saved
 * camera is returned. In subsequent calls, the previous camera returned
 * by this function should be passed as the parameter. If all saved cameras
 * have been iterated over, nullptr is returned. */
extern CAMERA *NextSavedCamera(CAMERA *previous);

extern void ParseCameraOptions(int *argc, char **argv);

extern void CameraDefaults();

#endif
