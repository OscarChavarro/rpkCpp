package vsdk.toolkit.app.options;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.scene.Camera;

public final class OptionsGroupCamera {
    private static final float DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5f;
    private static final Vector3Df DEFAULT_CAMERA_EYE_POSITION = new Vector3Df(10.0f, 0.0f, 0.0f);
    private static final Vector3Df DEFAULT_CAMERA_LOOK_POSITION = new Vector3Df(0.0f, 0.0f, 0.0f);
    private static final Vector3Df DEFAULT_CAMERA_UP_DIRECTION = new Vector3Df(0.0f, 0.0f, 1.0f);
    private static final ColorRgbMutable DEFAULT_BACKGROUND_COLOR = new ColorRgbMutable(0.0, 0.0, 0.0);
    private static Camera cameraState = new Camera();

    private static void cameraSetEyePositionOption(TypedOption.MutableValue<Vector3Df> val) {
        cameraState.setEyePosition((float)val.value.x(), (float)val.value.y(), (float)val.value.z());
    }

    private static void cameraSetLookPositionOption(TypedOption.MutableValue<Vector3Df> val) {
        cameraState.setLookPosition((float)val.value.x(), (float)val.value.y(), (float)val.value.z());
    }

    private static void cameraSetUpDirectionOption(TypedOption.MutableValue<Vector3Df> val) {
        cameraState.setUpDirection((float)val.value.x(), (float)val.value.y(), (float)val.value.z());
    }

    private static void cameraSetFieldOfViewOption(TypedOption.MutableValue<Float> val) {
        cameraState.setFieldOfView(val.value);
    }

    private static void cameraDefaults(Camera camera, int imageWidth, int imageHeight) {
        Vector3Df eyePosition = new Vector3Df(DEFAULT_CAMERA_EYE_POSITION.x(), DEFAULT_CAMERA_EYE_POSITION.y(), DEFAULT_CAMERA_EYE_POSITION.z());
        Vector3Df lookPosition = new Vector3Df(DEFAULT_CAMERA_LOOK_POSITION.x(), DEFAULT_CAMERA_LOOK_POSITION.y(), DEFAULT_CAMERA_LOOK_POSITION.z());
        Vector3Df upDirection = new Vector3Df(DEFAULT_CAMERA_UP_DIRECTION.x(), DEFAULT_CAMERA_UP_DIRECTION.y(), DEFAULT_CAMERA_UP_DIRECTION.z());
        ColorRgbMutable backgroundColorSelected = new ColorRgbMutable(
            DEFAULT_BACKGROUND_COLOR.getR(),
            DEFAULT_BACKGROUND_COLOR.getG(),
            DEFAULT_BACKGROUND_COLOR.getB());

        camera.set(
            eyePosition,
            lookPosition,
            upDirection,
            DEFAULT_CAMERA_FIELD_OF_VIEW,
            imageWidth,
            imageHeight,
            backgroundColorSelected);
    }

    private static boolean parseVector3(
        int argc,
        String[] argv,
        TypedOption.MutableValue<Vector3Df> value)
    {
        if (argc < 3 || argv == null || argv[0] == null || argv[1] == null || argv[2] == null) {
            return false;
        }
        try {
            value.value = value.value.withX(Float.parseFloat(argv[0]));
            value.value = value.value.withY(Float.parseFloat(argv[1]));
            value.value = value.value.withZ(Float.parseFloat(argv[2]));
            return true;
        }
        catch (NumberFormatException e) {
            return false;
        }
    }

    public static void cameraParseOptions(
        int[] argc,
        String[] argv,
        Camera camera,
        int imageWidth,
        int imageHeight)
    {
        TypedOption<Vector3Df> eyePointOpt = new TypedOption<>(
            "-eyepoint",
            TypedOption.reference(() -> cameraState.eyePosition, v -> cameraState.eyePosition = v),
            3,
            OptionsGroupCamera::cameraSetEyePositionOption,
            OptionsGroupCamera::parseVector3);
        TypedOption<Vector3Df> centerOpt = new TypedOption<>(
            "-center",
            TypedOption.reference(() -> cameraState.lookPosition, v -> cameraState.lookPosition = v),
            3,
            OptionsGroupCamera::cameraSetLookPositionOption,
            OptionsGroupCamera::parseVector3);
        TypedOption<Vector3Df> upDirOpt = new TypedOption<>(
            "-updir",
            TypedOption.reference(() -> cameraState.upDirection, v -> cameraState.upDirection = v),
            3,
            OptionsGroupCamera::cameraSetUpDirectionOption,
            OptionsGroupCamera::parseVector3);
        TypedOption<Float> fovOpt = new TypedOption<>(
            "-fov",
            TypedOption.reference(() -> cameraState.fieldOfVision, v -> cameraState.fieldOfVision = v),
            1,
            OptionsGroupCamera::cameraSetFieldOfViewOption,
            null);
        OptionBase[] cameraOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(eyePointOpt, 4),
            TypedOption.REGISTER_OPTION(centerOpt, 4),
            TypedOption.REGISTER_OPTION(upDirOpt, 3),
            TypedOption.REGISTER_OPTION(fovOpt, 4)
        };

        OptionsGroupCamera.cameraDefaults(cameraState, imageWidth, imageHeight);
        OptionGroup[] cameraGroups = new OptionGroup[] {
            new OptionGroup("camera", cameraOptions, 4)
        };
        OptionParser.parse(argc, argv, cameraGroups, 1);
        camera.set(
            cameraState.eyePosition,
            cameraState.lookPosition,
            cameraState.upDirection,
            cameraState.fieldOfVision,
            cameraState.xSize,
            cameraState.ySize,
            cameraState.background);
    }
}
