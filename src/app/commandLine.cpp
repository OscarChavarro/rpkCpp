#include "common/options.h"
#include "scene/Camera.h"
#include "app/commandLine.h"

#define DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS 4
#define DEFAULT_FORCE_ONE_SIDED true

static int globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
static int globalFileOptionsForceOneSidedSurfaces = 0;
static int globalYes = 1;
static int globalNo = 0;
static int globalOutputImageWidth = 1920;
static int globalOutputImageHeight = 1080;
static Camera globalCamera;

static void
mainForceOneSidedOption(void *value) {
    globalFileOptionsForceOneSidedSurfaces = *((int *) value);
}

static void
mainMonochromeOption(void *value) {
    globalNumberOfQuarterCircleDivisions = *(int *)value;
}

static void
commandLineImageWidthOption(void *value) {
    globalOutputImageWidth = *(int *)value;
}

static void
commandLineImageHeightOption(void *value) {
    globalOutputImageHeight = *(int *)value;
}

static CommandLineOptionDescription globalOptions[] = {
    {"-nqcdivs", 3, &GLOBAL_options_intType, &globalNumberOfQuarterCircleDivisions, DEFAULT_ACTION,
     "-nqcdivs <integer>\t: number of quarter circle divisions"},
    {"-force-onesided", 10, TYPELESS, &globalYes, mainForceOneSidedOption,
     "-force-onesided\t\t: force one-sided surfaces"},
    {"-dont-force-onesided", 14, TYPELESS, &globalNo, mainForceOneSidedOption,
     "-dont-force-onesided\t: allow two-sided surfaces"},
    {"-monochromatic", 5, TYPELESS, &globalYes, mainMonochromeOption,
     "-monochromatic \t\t: convert colors to shades of grey"},
    {"-width", 5, &GLOBAL_options_intType, &globalOutputImageWidth, commandLineImageWidthOption,
            "-width \t\t: image output width in pixels"},
    {"-height", 6, &GLOBAL_options_intType, &globalOutputImageHeight, commandLineImageHeightOption,
            "-width \t\t: image output width in pixels"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
commandLineGeneralProgramParseOptions(
    int *argc,
    char **argv,
    bool *oneSidedSurfaces,
    int *conicSubDivisions,
    int *imageOutputWidth,
    int *imageOutputHeight)
{
    globalFileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    parseGeneralOptions(globalOptions, argc, argv); // Order is important, this should be called last

    if ( globalFileOptionsForceOneSidedSurfaces != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = globalNumberOfQuarterCircleDivisions;
    *imageOutputWidth = globalOutputImageWidth;
    *imageOutputHeight = globalOutputImageHeight;
}

static void
cameraSetEyePositionOption(void *val) {
    Vector3D *v = (Vector3D *)val;
    globalCamera.setEyePosition(v->x, v->y, v->z);
}

static void
cameraSetLookPositionOption(void *val) {
    Vector3D *v = (Vector3D *)val;
    globalCamera.setLookPosition(v->x, v->y, v->z);
}

static void
cameraSetUpDirectionOption(void *val) {
    Vector3D *v = (Vector3D *)val;
    globalCamera.setUpDirection(v->x, v->y, v->z);
}

static void cameraSetFieldOfViewOption(void *val) {
    float *v = (float *)val;
    globalCamera.setFieldOfView(*v);
}

static CommandLineOptionDescription globalCameraOptions[] = {
    {"-eyepoint", 4, TVECTOR, &globalCamera.eyePosition, cameraSetEyePositionOption,
     "-eyepoint  <vector>\t: viewing position"},
    {"-center", 4, TVECTOR, &globalCamera.lookPosition, cameraSetLookPositionOption,
     "-center    <vector>\t: point looked at"},
    {"-updir", 3, TVECTOR, &globalCamera.upDirection, cameraSetUpDirectionOption,
     "-updir     <vector>\t: direction pointing up"},
    {"-fov", 4, Tfloat,  &globalCamera.fieldOfVision, cameraSetFieldOfViewOption,
     "-fov       <float> \t: field of view angle"},
    {nullptr, 0, TYPELESS, nullptr, nullptr, nullptr}
};

// Default virtual camera
#define DEFAULT_CAMERA_EYE_POSITION {10.0, 0.0, 0.0}
#define DEFAULT_CAMERA_LOOK_POSITION { 0.0, 0.0, 0.0}
#define DEFAULT_CAMERA_UP_DIRECTION { 0.0, 0.0, 1.0}
#define DEFAULT_CAMERA_FIELD_OF_VIEW 22.5
#define DEFAULT_BACKGROUND_COLOR {0.0, 0.0, 0.0}

static void
cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
    Vector3D eyePosition = DEFAULT_CAMERA_EYE_POSITION;
    Vector3D lookPosition = DEFAULT_CAMERA_LOOK_POSITION;
    Vector3D upDirection = DEFAULT_CAMERA_UP_DIRECTION;
    ColorRgb backgroundColor = DEFAULT_BACKGROUND_COLOR;

    camera->set(
            &eyePosition,
            &lookPosition,
            &upDirection,
            DEFAULT_CAMERA_FIELD_OF_VIEW,
            imageWidth,
            imageHeight,
            &backgroundColor);
}

void
cameraParseOptions(int *argc, char **argv, Camera *camera, int imageWidth, int imageHeight) {
    cameraDefaults(&globalCamera, imageWidth, imageHeight);
    parseGeneralOptions(globalCameraOptions, argc, argv);
    *camera = globalCamera;
}
