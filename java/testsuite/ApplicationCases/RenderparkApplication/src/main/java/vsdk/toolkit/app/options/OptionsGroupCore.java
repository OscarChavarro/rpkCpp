package vsdk.toolkit.app.options;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.ConstantColorBackground;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.SceneAccelerationMethod;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class OptionsGroupCore {
    private static final int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
    private static final boolean DEFAULT_FORCE_ONE_SIDED = true;
    private static final boolean DEFAULT_NATIVE_RNG_ENABLED = true;
    private static final ColorRgb DEFAULT_BACKGROUND_COLOR = new ColorRgb(0.0, 0.0, 0.0);

    private static int numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    private static int fileOptionsForceOneSidedSurfaces = 0;
    private static int outputImageWidth = 1920;
    private static int outputImageHeight = 1080;
    private static int glutDebugEnabled = 0;
    private static boolean nativeRandomNumberGeneratorEnabled = DEFAULT_NATIVE_RNG_ENABLED;
    private static EnumBackgroundMode backgroundMode = EnumBackgroundMode.NONE;
    private static ColorRgb backgroundColor = new ColorRgb(
        DEFAULT_BACKGROUND_COLOR.getR(),
        DEFAULT_BACKGROUND_COLOR.getG(),
        DEFAULT_BACKGROUND_COLOR.getB());
    private static SceneAccelerationMethod sceneAccelerationMethod = SceneAccelerationMethod.WALD2007_ACCELERATION;

    private OptionsGroupCore() {
    }

    /**
    -scene-acceleration voxel|wald2007|bruteforce: the spatial index of
    the scene query (SceneAccelerationMethod); wald2007 by default
    */
    private static void sceneAccelerationOption(TypedOption.MutableValue<String> value) {
        SceneAccelerationMethod method = SceneQuery.methodFromName(value.value);
        if ( method == null ) {
            Logger.reportMessage(null, Logger.ERROR, "", String.format(
                "Invalid scene acceleration '%s' (voxel, wald2007 or bruteforce)", value.value));
        }
        else {
            sceneAccelerationMethod = method;
        }
    }

    public static void parse(
        int[] argc,
        String[] argv,
        ParseRuntimeContext parseSession,
        Scene scene,
        RendererConfiguration renderOptions,
        ToneMappingContext toneMapOptions,
        int[] imageOutputWidth,
        int[] imageOutputHeight,
        boolean[] glutDebugEnabledOut,
        String[] toneMapNameOut)
    {
        boolean[] oneSidedSurfaces = new boolean[] {parseSession.singleSided};
        int[] conicSubDivisions = new int[] {parseSession.numberOfQuarterCircleDivisions};

        OptionsGroupCore.commandLineGeneralProgramParseOptions(
            argc,
            argv,
            oneSidedSurfaces,
            conicSubDivisions,
            imageOutputWidth,
            imageOutputHeight,
            glutDebugEnabledOut);
        scene.setAccelerationMethod(sceneAccelerationMethod);

        parseSession.singleSided = oneSidedSurfaces[0];
        parseSession.parserConfig.singleSided = oneSidedSurfaces[0];
        parseSession.numberOfQuarterCircleDivisions = conicSubDivisions[0];
        parseSession.parserConfig.numberOfQuarterCircleDivisions = conicSubDivisions[0];

        OptionsGroupRender.renderParseOptions(argc, argv, renderOptions);
        OptionsGroupToneMapping.toneMapParseOptions(argc, argv, toneMapNameOut, toneMapOptions);
        OptionsGroupCamera.cameraParseOptions(
            argc,
            argv,
            scene.camera,
            imageOutputWidth[0],
            imageOutputHeight[0]);
    }

    public static boolean isNativeRandomNumberGeneratorEnabled() {
        return nativeRandomNumberGeneratorEnabled;
    }

    public static Background createBackground() {
        return OptionsGroupCore.commandLineCreateBackground();
    }

    public static Background commandLineCreateBackground() {
        if ( backgroundMode == EnumBackgroundMode.SOLID ) {
            return new ConstantColorBackground(backgroundColor);
        }
        return null;
    }

    private static boolean commandLineParseFloat(String text, double[] value) {
        if ( text == null || value == null || value.length == 0 ) {
            return false;
        }

        try {
            value[0] = Double.parseDouble(text);
            return true;
        }
        catch ( NumberFormatException e ) {
            return false;
        }
    }

    private static boolean commandLineParseBackgroundColor(String rArg, String gArg, String bArg, ColorRgb[] color) {
        double[] red = new double[1];
        double[] green = new double[1];
        double[] blue = new double[1];

        if ( !OptionsGroupCore.commandLineParseFloat(rArg, red)
             || !OptionsGroupCore.commandLineParseFloat(gArg, green)
             || !OptionsGroupCore.commandLineParseFloat(bArg, blue) ) {
            return false;
        }

        if ( red[0] < 0.0 || red[0] > 1.0
             || green[0] < 0.0 || green[0] > 1.0
             || blue[0] < 0.0 || blue[0] > 1.0 ) {
            return false;
        }

        color[0] = new ColorRgb(red[0], green[0], blue[0]);
        return true;
    }

    private static void commandLineParseBackgroundOption(int[] argc, String[] argv) {
        if ( argc == null || argc.length == 0 || argv == null ) {
            return;
        }

        int writeIndex = 0;
        int readIndex = 0;
        while ( readIndex < argc[0] ) {
            String argument = argv[readIndex];
            if ( argument == null || !"-background".equals(argument) ) {
                argv[writeIndex++] = argv[readIndex++];
                continue;
            }

            if ( readIndex + 1 >= argc[0] ) {
                System.err.printf("Option '-background' requires a mode. Supported mode: solid.\n");
                readIndex += 1;
                continue;
            }

            String mode = argv[readIndex + 1];
            if ( !OptionTextUtils.equalsIgnoreCase(mode, "solid") ) {
                System.err.printf(
                    "Invalid background mode '%s'. Expected '-background solid <r> <g> <b>'.\n",
                    mode);
                readIndex += 2;
                continue;
            }

            if ( readIndex + 4 >= argc[0] ) {
                System.err.printf(
                    "Option '-background solid' requires three values in range [0.0, 1.0].\n");
                readIndex += 2;
                continue;
            }

            final ColorRgb[] parsedColor = new ColorRgb[] {new ColorRgb(0.0, 0.0, 0.0)};
            if ( !OptionsGroupCore.commandLineParseBackgroundColor(
                     argv[readIndex + 2],
                     argv[readIndex + 3],
                     argv[readIndex + 4],
                     parsedColor) ) {
                System.err.printf(
                    "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n");
            }
            else {
                backgroundMode = EnumBackgroundMode.SOLID;
                backgroundColor = parsedColor[0];
            }
            readIndex += 5;
        }

        while ( writeIndex < argc[0] ) {
            argv[writeIndex++] = null;
        }
        argc[0] = writeIndex;
    }

    private static void mainForceOneSidedOption(TypedOption.MutableValue<Integer> value) {
        fileOptionsForceOneSidedSurfaces = value.value;
    }

    private static void mainMonochromeOption(TypedOption.MutableValue<Integer> value) {
        numberOfQuarterCircleDivisions = value.value;
    }

    private static void setIntTrue(TypedOption.MutableValue<Integer> value) {
        value.value = 1;
    }

    private static void mainNativeRNGOption(TypedOption.MutableValue<Boolean> value) {
        nativeRandomNumberGeneratorEnabled = value.value;
    }

    public static void commandLineGeneralProgramParseOptions(
        int[] argc,
        String[] argv,
        boolean[] oneSidedSurfaces,
        int[] conicSubDivisions,
        int[] imageOutputWidth,
        int[] imageOutputHeight,
        boolean[] glutDebugEnabledOut)
    {
        EnumAppOptions appOptions = new EnumAppOptions(
            outputImageWidth, outputImageHeight, numberOfQuarterCircleDivisions,
            1, 0, 0, DEFAULT_NATIVE_RNG_ENABLED);

        TypedOption<Integer> widthOpt = new TypedOption<>(
            "-width",
            TypedOption.reference(() -> appOptions.width, v -> appOptions.width = v),
            1,
            null,
            null);
        TypedOption<Integer> heightOpt = new TypedOption<>(
            "-height",
            TypedOption.reference(() -> appOptions.height, v -> appOptions.height = v),
            1,
            null,
            null);
        TypedOption<Integer> nqcdivsOpt = new TypedOption<>(
            "-nqcdivs",
            TypedOption.reference(() -> appOptions.nqcdivs, v -> appOptions.nqcdivs = v),
            1,
            null,
            null);
        TypedOption<Integer> forceOneSidedOpt = new TypedOption<>(
            "-force-onesided",
            TypedOption.reference(() -> appOptions.yesValue, v -> appOptions.yesValue = v),
            0,
            OptionsGroupCore::mainForceOneSidedOption,
            null);
        TypedOption<Integer> dontForceOneSidedOpt = new TypedOption<>(
            "-dont-force-onesided",
            TypedOption.reference(() -> appOptions.noValue, v -> appOptions.noValue = v),
            0,
            OptionsGroupCore::mainForceOneSidedOption,
            null);
        TypedOption<Integer> monochromaticOpt = new TypedOption<>(
            "-monochromatic",
            TypedOption.reference(() -> appOptions.yesValue, v -> appOptions.yesValue = v),
            0,
            OptionsGroupCore::mainMonochromeOption,
            null);
        TypedOption<Integer> glutDebugOpt = new TypedOption<>(
            "-glutDebug",
            TypedOption.reference(() -> appOptions.debug, v -> appOptions.debug = v),
            0,
            OptionsGroupCore::setIntTrue,
            null);
        TypedOption<Boolean> nativeRNGOpt = new TypedOption<>(
            "-nativeRNG",
            TypedOption.reference(() -> appOptions.nativeRNG, v -> appOptions.nativeRNG = v),
            1,
            OptionsGroupCore::mainNativeRNGOption,
            null);
        TypedOption.ValueRef<String> sceneAccelerationName = TypedOption.valueRef((String)null);
        TypedOption<String> sceneAccelerationOpt = new TypedOption<>(
            "-scene-acceleration",
            sceneAccelerationName,
            1,
            OptionsGroupCore::sceneAccelerationOption,
            null);
        OptionBase[] registry = new OptionBase[] {
            TypedOption.REGISTER_OPTION(widthOpt, 5),
            TypedOption.REGISTER_OPTION(heightOpt, 6),
            TypedOption.REGISTER_OPTION(nqcdivsOpt, 3),
            TypedOption.REGISTER_OPTION(forceOneSidedOpt, 10),
            TypedOption.REGISTER_OPTION(dontForceOneSidedOpt, 14),
            TypedOption.REGISTER_OPTION(monochromaticOpt, 5),
            TypedOption.REGISTER_OPTION(glutDebugOpt, 6),
            TypedOption.REGISTER_OPTION(nativeRNGOpt, 9),
            TypedOption.REGISTER_OPTION(sceneAccelerationOpt, 8)
        };

        fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED ? 1 : 0;
        numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
        backgroundMode = EnumBackgroundMode.NONE;
        backgroundColor = new ColorRgb(
            DEFAULT_BACKGROUND_COLOR.getR(),
            DEFAULT_BACKGROUND_COLOR.getG(),
            DEFAULT_BACKGROUND_COLOR.getB());
        glutDebugEnabled = appOptions.debug;
        nativeRandomNumberGeneratorEnabled = appOptions.nativeRNG;
        sceneAccelerationMethod = SceneAccelerationMethod.WALD2007_ACCELERATION;

        OptionsGroupCore.commandLineParseBackgroundOption(argc, argv);

        OptionGroup[] generalGroups = new OptionGroup[] {
            new OptionGroup("global", registry, 9)
        };
        OptionParser.parse(argc, argv, generalGroups, 1);

        outputImageWidth = appOptions.width;
        outputImageHeight = appOptions.height;
        numberOfQuarterCircleDivisions = appOptions.nqcdivs;
        glutDebugEnabled = appOptions.debug;
        nativeRandomNumberGeneratorEnabled = appOptions.nativeRNG;

        if ( oneSidedSurfaces != null && oneSidedSurfaces.length > 0 ) {
            oneSidedSurfaces[0] = fileOptionsForceOneSidedSurfaces != 0;
        }
        if ( conicSubDivisions != null && conicSubDivisions.length > 0 ) {
            conicSubDivisions[0] = numberOfQuarterCircleDivisions;
        }
        if ( imageOutputWidth != null && imageOutputWidth.length > 0 ) {
            imageOutputWidth[0] = outputImageWidth;
        }
        if ( imageOutputHeight != null && imageOutputHeight.length > 0 ) {
            imageOutputHeight[0] = outputImageHeight;
        }
        if ( glutDebugEnabledOut != null && glutDebugEnabledOut.length > 0 ) {
            glutDebugEnabledOut[0] = glutDebugEnabled != 0;
        }

    }
}
