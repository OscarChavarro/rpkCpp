package vsdk.toolkit.app.options;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.ConstantColorBackground;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class OptionsGroupCore {
    private static final int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
    private static final boolean DEFAULT_FORCE_ONE_SIDED = true;
    private static final ColorRgb DEFAULT_BACKGROUND_COLOR = new ColorRgb(0.0, 0.0, 0.0);

    private static int numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    private static int fileOptionsForceOneSidedSurfaces = 0;
    private static int outputImageWidth = 1920;
    private static int outputImageHeight = 1080;
    private static int glutDebugEnabled = 0;
    private static EnumBackgroundMode backgroundMode = EnumBackgroundMode.NONE;
    private static ColorRgb backgroundColor = new ColorRgb(
        DEFAULT_BACKGROUND_COLOR.getR(),
        DEFAULT_BACKGROUND_COLOR.getG(),
        DEFAULT_BACKGROUND_COLOR.getB());

    private OptionsGroupCore() {
    }

    public static void parse(
        int[] argc,
        String[] argv,
        ParseRuntimeContext parseSession,
        Scene scene,
        RenderOptions renderOptions,
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

    public static Background createBackground() {
        return OptionsGroupCore.commandLineCreateBackground();
    }

    public static Background commandLineCreateBackground() {
        if ( backgroundMode == EnumBackgroundMode.SOLID ) {
            return new ConstantColorBackground(backgroundColor);
        }
        return null;
    }

    private static boolean commandLineParseFloat(String text, float[] value) {
        if ( text == null || value == null || value.length == 0 ) {
            return false;
        }

        try {
            value[0] = Float.parseFloat(text);
            return true;
        }
        catch ( NumberFormatException e ) {
            return false;
        }
    }

    private static boolean commandLineParseBackgroundColor(String rArg, String gArg, String bArg, ColorRgb color) {
        float[] red = new float[1];
        float[] green = new float[1];
        float[] blue = new float[1];

        if ( !OptionsGroupCore.commandLineParseFloat(rArg, red)
             || !OptionsGroupCore.commandLineParseFloat(gArg, green)
             || !OptionsGroupCore.commandLineParseFloat(bArg, blue) ) {
            return false;
        }

        if ( red[0] < 0.0f || red[0] > 1.0f
             || green[0] < 0.0f || green[0] > 1.0f
             || blue[0] < 0.0f || blue[0] > 1.0f ) {
            return false;
        }

        color.set(red[0], green[0], blue[0]);
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

            ColorRgb parsedColor = new ColorRgb();
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
                backgroundColor = parsedColor;
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

    public static void commandLineGeneralProgramParseOptions(
        int[] argc,
        String[] argv,
        boolean[] oneSidedSurfaces,
        int[] conicSubDivisions,
        int[] imageOutputWidth,
        int[] imageOutputHeight,
        boolean[] glutDebugEnabledOut)
    {
        EnumAppOptions appOptions = new EnumAppOptions();
        appOptions.width = outputImageWidth;
        appOptions.height = outputImageHeight;
        appOptions.nqcdivs = numberOfQuarterCircleDivisions;
        appOptions.yesValue = 1;
        appOptions.noValue = 0;
        appOptions.debug = 0;

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
        OptionBase[] registry = new OptionBase[] {
            TypedOption.REGISTER_OPTION(widthOpt, 5),
            TypedOption.REGISTER_OPTION(heightOpt, 6),
            TypedOption.REGISTER_OPTION(nqcdivsOpt, 3),
            TypedOption.REGISTER_OPTION(forceOneSidedOpt, 10),
            TypedOption.REGISTER_OPTION(dontForceOneSidedOpt, 14),
            TypedOption.REGISTER_OPTION(monochromaticOpt, 5),
            TypedOption.REGISTER_OPTION(glutDebugOpt, 6)
        };

        fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED ? 1 : 0;
        numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
        backgroundMode = EnumBackgroundMode.NONE;
        backgroundColor = new ColorRgb(
            DEFAULT_BACKGROUND_COLOR.getR(),
            DEFAULT_BACKGROUND_COLOR.getG(),
            DEFAULT_BACKGROUND_COLOR.getB());
        glutDebugEnabled = appOptions.debug;

        OptionsGroupCore.commandLineParseBackgroundOption(argc, argv);

        OptionGroup[] generalGroups = new OptionGroup[] {
            new OptionGroup("global", registry, 7)
        };
        OptionParser.parse(argc, argv, generalGroups, 1);

        outputImageWidth = appOptions.width;
        outputImageHeight = appOptions.height;
        numberOfQuarterCircleDivisions = appOptions.nqcdivs;
        glutDebugEnabled = appOptions.debug;

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
