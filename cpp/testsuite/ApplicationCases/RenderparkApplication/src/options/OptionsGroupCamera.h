#ifndef COMMAND_LINE_CAMERA_OPTIONS_GROUP__
#define COMMAND_LINE_CAMERA_OPTIONS_GROUP__

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
    static const Vector3D DEFAULT_CAMERA_EYE_POSITION;
    static const Vector3D DEFAULT_CAMERA_LOOK_POSITION;
    static const Vector3D DEFAULT_CAMERA_UP_DIRECTION;
    static const ColorRgbMutable DEFAULT_BACKGROUND_COLOR;
    static Camera cameraState;

    static void cameraSetEyePositionOption(Vector3D &val);
    static void cameraSetLookPositionOption(Vector3D &val);
    static void cameraSetUpDirectionOption(Vector3D &val);
    static void cameraSetFieldOfViewOption(float &val);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static bool parseVector3(int argc, char **argv, Vector3D &value);
};

#endif
