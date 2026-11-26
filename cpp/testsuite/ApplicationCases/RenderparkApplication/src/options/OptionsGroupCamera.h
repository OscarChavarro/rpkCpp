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
    static constexpr double DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5;
    static const Vector3Dd DEFAULT_CAMERA_EYE_POSITION;
    static const Vector3Dd DEFAULT_CAMERA_LOOK_POSITION;
    static const Vector3Dd DEFAULT_CAMERA_UP_DIRECTION;
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;
    static Camera cameraState;

    static void cameraSetEyePositionOption(Vector3Dd &val);
    static void cameraSetLookPositionOption(Vector3Dd &val);
    static void cameraSetUpDirectionOption(Vector3Dd &val);
    static void cameraSetFieldOfViewOption(double &val);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static bool parseVector3(int argc, char **argv, Vector3Dd &value);
};

#endif
