#include <cstdlib>

#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "options/OptionsGroupCamera.h"

const Vector3D OptionsGroupCamera::DEFAULT_CAMERA_EYE_POSITION(10.0, 0.0, 0.0);
const Vector3D OptionsGroupCamera::DEFAULT_CAMERA_LOOK_POSITION(0.0, 0.0, 0.0);
const Vector3D OptionsGroupCamera::DEFAULT_CAMERA_UP_DIRECTION(0.0, 0.0, 1.0);
const ColorRgb OptionsGroupCamera::DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
Camera OptionsGroupCamera::cameraState;

void
OptionsGroupCamera::cameraSetEyePositionOption(Vector3D &val) {
    cameraState.setEyePosition(val.x, val.y, val.z);
}

void
OptionsGroupCamera::cameraSetLookPositionOption(Vector3D &val) {
    cameraState.setLookPosition(val.x, val.y, val.z);
}

void
OptionsGroupCamera::cameraSetUpDirectionOption(Vector3D &val) {
    cameraState.setUpDirection(val.x, val.y, val.z);
}

void
OptionsGroupCamera::cameraSetFieldOfViewOption(float &val) {
    cameraState.setFieldOfView(val);
}

void
OptionsGroupCamera::cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
    Vector3D eyePosition = DEFAULT_CAMERA_EYE_POSITION;
    Vector3D lookPosition = DEFAULT_CAMERA_LOOK_POSITION;
    Vector3D upDirection = DEFAULT_CAMERA_UP_DIRECTION;
    ColorRgb backgroundColorSelected = DEFAULT_BACKGROUND_COLOR;

    camera->set(
        &eyePosition,
        &lookPosition,
        &upDirection,
        DEFAULT_CAMERA_FIELD_OF_VIEW,
        imageWidth,
        imageHeight,
        &backgroundColorSelected);
}

void
OptionsGroupCamera::cameraParseOptions(
        int *argc,
        char **argv,
        Camera *camera,
        int imageWidth,
        int imageHeight)
{
    TypedOption<Vector3D> eyePointOpt = {"-eyepoint", &cameraState.eyePosition, 3, OptionsGroupCamera::cameraSetEyePositionOption, OptionsGroupCamera::parseVector3};
    TypedOption<Vector3D> centerOpt = {"-center", &cameraState.lookPosition, 3, OptionsGroupCamera::cameraSetLookPositionOption, OptionsGroupCamera::parseVector3};
    TypedOption<Vector3D> upDirOpt = {"-updir", &cameraState.upDirection, 3, OptionsGroupCamera::cameraSetUpDirectionOption, OptionsGroupCamera::parseVector3};
    TypedOption<float> fovOpt = {"-fov", &cameraState.fieldOfVision, 1, OptionsGroupCamera::cameraSetFieldOfViewOption, nullptr};
    OptionBase cameraOptions[] = {
        REGISTER_OPTION(Vector3D, eyePointOpt, 4),
        REGISTER_OPTION(Vector3D, centerOpt, 4),
        REGISTER_OPTION(Vector3D, upDirOpt, 3),
        REGISTER_OPTION(float, fovOpt, 4)
    };

    OptionsGroupCamera::cameraDefaults(&cameraState, imageWidth, imageHeight);
    OptionGroup cameraGroups[] = {
        OptionGroup("camera", cameraOptions, 4)
    };
    OptionParser<OptionBase>::parse(argc, argv, cameraGroups, 1);
    *camera = cameraState;
}

bool
OptionsGroupCamera::parseVector3(int argc, char **argv, Vector3D &value) {
    if ( argc < 3 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr || argv[2] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
    value.x = strtof(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    value.y = strtof(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    value.z = strtof(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    return true;
}
