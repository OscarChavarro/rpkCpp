#include <cstdlib>

#include "common/commandLineOptions/OptionParser.h"
#include "app/options/CommandLine.h"

namespace {

bool parseVector3(int argc, char **argv, Vector3D &value) {
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

}

void
CommandLine::cameraSetEyePositionOption(Vector3D &val) {
    cameraState.setEyePosition(val.x, val.y, val.z);
}

void
CommandLine::cameraSetLookPositionOption(Vector3D &val) {
    cameraState.setLookPosition(val.x, val.y, val.z);
}

void
CommandLine::cameraSetUpDirectionOption(Vector3D &val) {
    cameraState.setUpDirection(val.x, val.y, val.z);
}

void
CommandLine::cameraSetFieldOfViewOption(float &val) {
    cameraState.setFieldOfView(val);
}

void
CommandLine::cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
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
CommandLine::cameraParseOptions(
        int *argc,
        char **argv,
        Camera *camera,
        int imageWidth,
        int imageHeight)
{
    TypedOption<Vector3D> eyePointOpt = {"-eyepoint", &cameraState.eyePosition, 3, CommandLine::cameraSetEyePositionOption, parseVector3};
    TypedOption<Vector3D> centerOpt = {"-center", &cameraState.lookPosition, 3, CommandLine::cameraSetLookPositionOption, parseVector3};
    TypedOption<Vector3D> upDirOpt = {"-updir", &cameraState.upDirection, 3, CommandLine::cameraSetUpDirectionOption, parseVector3};
    TypedOption<float> fovOpt = {"-fov", &cameraState.fieldOfVision, 1, CommandLine::cameraSetFieldOfViewOption, nullptr};
    OptionBase cameraOptions[] = {
        REGISTER_OPTION(Vector3D, eyePointOpt, 4),
        REGISTER_OPTION(Vector3D, centerOpt, 4),
        REGISTER_OPTION(Vector3D, upDirOpt, 3),
        REGISTER_OPTION(float, fovOpt, 4)
    };

    CommandLine::cameraDefaults(&cameraState, imageWidth, imageHeight);
    OptionGroup cameraGroups[] = {
        OptionGroup("camera", cameraOptions, 4)
    };
    OptionParser<OptionBase>::parse(argc, argv, cameraGroups, 1);
    *camera = cameraState;
}
