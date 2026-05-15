#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef CMMND_LINE_CMR_OPTNS_GRP
#define CMMND_LINE_CMR_OPTNS_GRP

#include "scene/Camera.h"
#include "common/color/ColorRgb.h"

class OptionsGroupCamera{ public:
    static void cameraParseOptions( int *argc, char **argv, Camera *camera, int imageWidth, int imageHeight);

  private:
    #define DEFAULT_CAMERA_FIELD_OF_VIEW ((float)22.5)
    static const Vector3D DEFAULT_CAMERA_EYE_POSITION;
    static const Vector3D DEFAULT_CAMERA_LOOK_POSITION;
    static const Vector3D DEFAULT_CAMERA_UP_DIRECTION;
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;
    static Camera cameraState;

    static void cameraSetEyePositionOption(Vector3D &val);
    static void cameraSetLookPositionOption(Vector3D &val);
    static void cameraSetUpDirectionOption(Vector3D &val);
    static void cameraSetFieldOfViewOption(float &val);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static bool parseVector3(int argc, char **argv, Vector3D &value);
};

#endif
