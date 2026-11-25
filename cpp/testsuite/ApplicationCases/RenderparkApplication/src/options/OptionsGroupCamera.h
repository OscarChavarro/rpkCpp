#ifndef __OPTIONS_GROUP_CAMERA__
#define __OPTIONS_GROUP_CAMERA__

#include "vsdk/toolkit/scene/Camera.h"

class OptionsGroupCamera final {
  public:
    static void cameraParseOptions(
        int *argc,
        char **argv,
        Camera *camera,
        int imageWidth,
        int imageHeight);

  private:
    static constexpr float DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5F;
    static const Vector3Df DEFAULT_CAMERA_EYE_POSITION;
    static const Vector3Df DEFAULT_CAMERA_LOOK_POSITION;
    static const Vector3Df DEFAULT_CAMERA_UP_DIRECTION;
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;
    static Camera cameraState;

    static void cameraSetEyePositionOption(Vector3Df &val);
    static void cameraSetLookPositionOption(Vector3Df &val);
    static void cameraSetUpDirectionOption(Vector3Df &val);
    static void cameraSetFieldOfViewOption(float &val);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static bool parseVector3(int argc, char **argv, Vector3Df &value);
};

#endif
