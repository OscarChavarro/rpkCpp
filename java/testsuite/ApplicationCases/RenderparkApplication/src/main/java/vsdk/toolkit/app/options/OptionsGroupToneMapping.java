package vsdk.toolkit.app.options;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMapAdaptationMethod;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class OptionsGroupToneMapping {
    private static final int TONE_MAPPING_METHODS_STRING_LENGTH = 1000;

    private static String toneMappingMethodsString;
    private static String[] toneMapName;
    private static ToneMappingContext toneMapOptions;

    private OptionsGroupToneMapping() {
    }

    private static void makeToneMappingMethodsString() {
        toneMappingMethodsString =
           "-tonemapping <method>: Set tone mapping method\n"
           + "\tmethods: Lightness            Lightness Mapping (default)\n"
           + "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
           + "\t         Ward                 Ward's Mapping\n"
           + "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
           + "\t         Ferwerda             Partial Ferwerda's Mapping";

        if ( toneMappingMethodsString.length() > TONE_MAPPING_METHODS_STRING_LENGTH ) {
            toneMappingMethodsString = toneMappingMethodsString.substring(0, TONE_MAPPING_METHODS_STRING_LENGTH);
        }
    }

    private static void toneMappingMethodOption(TypedOption.MutableValue<String> name) {
        if ( toneMapName != null && toneMapName.length > 0 ) {
            toneMapName[0] = name.value;
        }
    }

    private static void brightnessAdjustOption(TypedOption.MutableValue<Double> value) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::brightnessAdjustOption", "ToneMappingContext not set");
        }
        toneMapOptions.pow_bright_adjust = (double)Math.pow(2.0, (double) toneMapOptions.brightness_adjust);
    }

    private static void redChromaOption(TypedOption.MutableValue<Vector3Dd> value) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::redChromaOption", "ToneMappingContext not set");
        }
        final Cie c = toneMapOptions.getColorSpace();
        toneMapOptions.setColorSpace(new Cie(
            value.value.x(), value.value.y(),
            c.getGreenX(), c.getGreenY(),
            c.getBlueX(), c.getBlueY(),
            c.getWhiteX(), c.getWhiteY()));
    }

    private static void greenChromaOption(TypedOption.MutableValue<Vector3Dd> value) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::greenChromaOption", "ToneMappingContext not set");
        }
        final Cie c = toneMapOptions.getColorSpace();
        toneMapOptions.setColorSpace(new Cie(
            c.getRedX(), c.getRedY(),
            value.value.x(), value.value.y(),
            c.getBlueX(), c.getBlueY(),
            c.getWhiteX(), c.getWhiteY()));
    }

    private static void blueChromaOption(TypedOption.MutableValue<Vector3Dd> value) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::blueChromaOption", "ToneMappingContext not set");
        }
        final Cie c = toneMapOptions.getColorSpace();
        toneMapOptions.setColorSpace(new Cie(
            c.getRedX(), c.getRedY(),
            c.getGreenX(), c.getGreenY(),
            value.value.x(), value.value.y(),
            c.getWhiteX(), c.getWhiteY()));
    }

    private static void whiteChromaOption(TypedOption.MutableValue<Vector3Dd> value) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::whiteChromaOption", "ToneMappingContext not set");
        }
        final Cie c = toneMapOptions.getColorSpace();
        toneMapOptions.setColorSpace(new Cie(
            c.getRedX(), c.getRedY(),
            c.getGreenX(), c.getGreenY(),
            c.getBlueX(), c.getBlueY(),
            value.value.x(), value.value.y()));
    }

    private static void toneMappingCommandLineOptionDescAdaptMethodOption(TypedOption.MutableValue<String> name) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::toneMappingCommandLineOptionDescAdaptMethodOption", "ToneMappingContext not set");
        }
        if ( OptionTextUtils.equalsIgnoreCasePrefix(name.value, "average", 2) ) {
            toneMapOptions.staticAdaptationMethod = ToneMapAdaptationMethod.TMA_AVERAGE;
        }
        else if ( OptionTextUtils.equalsIgnoreCasePrefix(name.value, "median", 2) ) {
            toneMapOptions.staticAdaptationMethod = ToneMapAdaptationMethod.TMA_MEDIAN;
        }
        else {
            Logger.reportMessage(null, Logger.ERROR, "", String.format("Invalid adaptation estimate method '%s'", name.value));
        }
    }

    private static void gammaOption(TypedOption.MutableValue<Double> gam) {
        if ( toneMapOptions == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "CommandLineToneMappingOptionsGroup::gammaOption", "ToneMappingContext not set");
        }
        toneMapOptions.gamma = new ColorRgb(gam.value, gam.value, gam.value);
    }

    public static void toneMapParseOptions(
        int[] argc,
        String[] argv,
        String[] toneMapNameOut,
        ToneMappingContext toneMapOptionsContext)
    {
        TypedOption.ValueRef<String> toneMapMethodName = TypedOption.valueRef((String)null);
        TypedOption.ValueRef<String> adaptMethodName = TypedOption.valueRef((String)null);
        Vector3Dd redChromaticityValue = new Vector3Dd(0.0, 0.0, 0.0);
        Vector3Dd greenChromaticityValue = new Vector3Dd(0.0, 0.0, 0.0);
        Vector3Dd blueChromaticityValue = new Vector3Dd(0.0, 0.0, 0.0);
        Vector3Dd whiteChromaticityValue = new Vector3Dd(0.0, 0.0, 0.0);

        TypedOption<String> toneMappingOpt = new TypedOption<>(
            "-tonemapping",
            toneMapMethodName,
            1,
            OptionsGroupToneMapping::toneMappingMethodOption,
            null);
        TypedOption<Double> brightnessAdjustOpt = new TypedOption<>(
            "-brightness-adjust",
            TypedOption.reference(() -> toneMapOptionsContext.brightness_adjust, v -> toneMapOptionsContext.brightness_adjust = v),
            1,
            OptionsGroupToneMapping::brightnessAdjustOption,
            null);
        TypedOption<String> adaptOpt = new TypedOption<>(
            "-adapt",
            adaptMethodName,
            1,
            OptionsGroupToneMapping::toneMappingCommandLineOptionDescAdaptMethodOption,
            null);
        TypedOption<Double> lwaOpt = new TypedOption<>(
            "-lwa",
            TypedOption.reference(() -> toneMapOptionsContext.realWorldAdaptionLuminance, v -> toneMapOptionsContext.realWorldAdaptionLuminance = v),
            1,
            null,
            null);
        TypedOption<Double> ldmaxOpt = new TypedOption<>(
            "-ldmax",
            TypedOption.reference(() -> toneMapOptionsContext.maximumDisplayLuminance, v -> toneMapOptionsContext.maximumDisplayLuminance = v),
            1,
            null,
            null);
        TypedOption<Double> cmaxOpt = new TypedOption<>(
            "-cmax",
            TypedOption.reference(() -> toneMapOptionsContext.maximumDisplayContrast, v -> toneMapOptionsContext.maximumDisplayContrast = v),
            1,
            null,
            null);
        TypedOption<Double> gammaOpt = new TypedOption<>(
            "-gamma",
            TypedOption.reference(() -> (double)toneMapOptionsContext.gamma.getR(), v -> toneMapOptionsContext.gamma = new ColorRgb(v, toneMapOptionsContext.gamma.getG(), toneMapOptionsContext.gamma.getB())),
            1,
            OptionsGroupToneMapping::gammaOption,
            null);
        TypedOption<ColorRgb> rgbGammaOpt = new TypedOption<>(
            "-rgbgamma",
            TypedOption.reference(() -> toneMapOptionsContext.gamma, v -> toneMapOptionsContext.gamma = v),
            3,
            null,
            OptionsGroupToneMapping::parseColor3);
        TypedOption<Vector3Dd> redOpt = new TypedOption<>(
            "-red",
            TypedOption.valueRef(redChromaticityValue),
            2,
            OptionsGroupToneMapping::redChromaOption,
            OptionsGroupToneMapping::parseCieXy);
        TypedOption<Vector3Dd> greenOpt = new TypedOption<>(
            "-green",
            TypedOption.valueRef(greenChromaticityValue),
            2,
            OptionsGroupToneMapping::greenChromaOption,
            OptionsGroupToneMapping::parseCieXy);
        TypedOption<Vector3Dd> blueOpt = new TypedOption<>(
            "-blue",
            TypedOption.valueRef(blueChromaticityValue),
            2,
            OptionsGroupToneMapping::blueChromaOption,
            OptionsGroupToneMapping::parseCieXy);
        TypedOption<Vector3Dd> whiteOpt = new TypedOption<>(
            "-white",
            TypedOption.valueRef(whiteChromaticityValue),
            2,
            OptionsGroupToneMapping::whiteChromaOption,
            OptionsGroupToneMapping::parseCieXy);
        OptionBase[] toneMappingOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(toneMappingOpt, 4),
            TypedOption.REGISTER_OPTION(brightnessAdjustOpt, 4),
            TypedOption.REGISTER_OPTION(adaptOpt, 5),
            TypedOption.REGISTER_OPTION(lwaOpt, 3),
            TypedOption.REGISTER_OPTION(ldmaxOpt, 5),
            TypedOption.REGISTER_OPTION(cmaxOpt, 4),
            TypedOption.REGISTER_OPTION(gammaOpt, 4),
            TypedOption.REGISTER_OPTION(rgbGammaOpt, 4),
            TypedOption.REGISTER_OPTION(redOpt, 4),
            TypedOption.REGISTER_OPTION(greenOpt, 4),
            TypedOption.REGISTER_OPTION(blueOpt, 4),
            TypedOption.REGISTER_OPTION(whiteOpt, 4)
        };

        OptionsGroupToneMapping.toneMapName = toneMapNameOut;
        OptionsGroupToneMapping.toneMapOptions = toneMapOptionsContext;
        OptionsGroupToneMapping.makeToneMappingMethodsString();
        OptionGroup[] toneMappingGroups = new OptionGroup[] {
            new OptionGroup("toneMapping", toneMappingOptions, 12)
        };
        OptionParser.parse(argc, argv, toneMappingGroups, 1);
        ToneMap.recomputeGammaTables(toneMapOptionsContext, OptionsGroupToneMapping.toneMapOptions.gamma);
        OptionsGroupToneMapping.toneMapOptions = null;
        OptionsGroupToneMapping.toneMapName = null;
    }

    private static boolean parseColor3(
        int argc,
        String[] argv,
        TypedOption.MutableValue<ColorRgb> value)
    {
        if ( argc < 3 || argv == null || argv[0] == null || argv[1] == null || argv[2] == null ) {
            return false;
        }
        try {
            final double r = Double.parseDouble(argv[0]);
            final double g = Double.parseDouble(argv[1]);
            final double b = Double.parseDouble(argv[2]);
            value.value = new ColorRgb(r, g, b);
            return true;
        }
        catch ( NumberFormatException e ) {
            return false;
        }
    }

    private static boolean parseCieXy(
        int argc,
        String[] argv,
        TypedOption.MutableValue<Vector3Dd> value)
    {
        if ( argc < 2 || argv == null || argv[0] == null || argv[1] == null ) {
            return false;
        }
        try {
            value.value = value.value.withX(Double.parseDouble(argv[0]));
            value.value = value.value.withY(Double.parseDouble(argv[1]));
            value.value = value.value.withZ(0.0);
            return true;
        }
        catch ( NumberFormatException e ) {
            return false;
        }
    }
}
