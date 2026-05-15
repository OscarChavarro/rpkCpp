package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.stochasticRaytracing.RayTracingLightMode;
import vsdk.toolkit.raycasting.stochasticRaytracing.RayTracingRadMode;
import vsdk.toolkit.raycasting.stochasticRaytracing.RayTracingSamplingMode;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRayTracingState;

public final class OptionsGroupStochasticRaytracing {
    private static final class EnumBinding<T extends Enum<T>> {
        public TypedOption.Reference<T> target;
        public EnumDesc[] values;
        public Class<T> enumType;
    }

    private static EnumDesc[] rayTracingRadianceModeValues = new EnumDesc[] {
        new EnumDesc(RayTracingRadMode.STORED_NONE.ordinal(), "none", 2),
        new EnumDesc(RayTracingRadMode.STORED_DIRECT.ordinal(), "direct", 2),
        new EnumDesc(RayTracingRadMode.STORED_INDIRECT.ordinal(), "indirect", 2),
        new EnumDesc(RayTracingRadMode.STORED_PHOTON_MAP.ordinal(), "photonmap", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] rayTracingLightModeValues = new EnumDesc[] {
        new EnumDesc(RayTracingLightMode.POWER_LIGHTS.ordinal(), "power", 2),
        new EnumDesc(RayTracingLightMode.IMPORTANT_LIGHTS.ordinal(), "important", 2),
        new EnumDesc(RayTracingLightMode.ALL_LIGHTS.ordinal(), "all", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] rayTracingSamplingModeValues = new EnumDesc[] {
        new EnumDesc(RayTracingSamplingMode.BRDF_SAMPLING.ordinal(), "bsdf", 2),
        new EnumDesc(RayTracingSamplingMode.CLASSICAL_SAMPLING.ordinal(), "classical", 2),
        new EnumDesc(0, null, 0)
    };

    private static <T extends Enum<T>> boolean parseEnumBinding(
        int argc,
        String[] argv,
        TypedOption.MutableValue<EnumBinding<T>> bindingValue)
    {
        EnumBinding<T> binding = bindingValue.value;
        if (argc < 1 || argv == null || argv[0] == null || binding == null || binding.target == null || binding.values == null) {
            return false;
        }
        for (int i = 0; binding.values[i].name != null; i++) {
            if (OptionTextUtils.equalsIgnoreCasePrefix(argv[0], binding.values[i].name, binding.values[i].abbrev)) {
                T[] constants = binding.enumType.getEnumConstants();
                int ordinal = binding.values[i].value;
                if (ordinal < 0 || ordinal >= constants.length) {
                    return false;
                }
                binding.target.set(constants[ordinal]);
                return true;
            }
        }
        return false;
    }

    private static void setIntTrue(TypedOption.MutableValue<Integer> value) {
        value.value = 1;
    }

    private static void setIntFalse(TypedOption.MutableValue<Integer> value) {
        value.value = 0;
    }

    public static void parse(
        int[] argc,
        String[] argv,
        StochasticRayTracingState stochasticRayTracingState)
    {
        EnumBinding<RayTracingRadMode> radModeBinding = new EnumBinding<>();
        radModeBinding.target = TypedOption.reference(() -> stochasticRayTracingState.radMode, v -> stochasticRayTracingState.radMode = v);
        radModeBinding.values = rayTracingRadianceModeValues;
        radModeBinding.enumType = RayTracingRadMode.class;

        EnumBinding<RayTracingLightMode> lightModeBinding = new EnumBinding<>();
        lightModeBinding.target = TypedOption.reference(() -> stochasticRayTracingState.lightMode, v -> stochasticRayTracingState.lightMode = v);
        lightModeBinding.values = rayTracingLightModeValues;
        lightModeBinding.enumType = RayTracingLightMode.class;

        EnumBinding<RayTracingSamplingMode> samplingModeBinding = new EnumBinding<>();
        samplingModeBinding.target = TypedOption.reference(() -> stochasticRayTracingState.reflectionSampling, v -> stochasticRayTracingState.reflectionSampling = v);
        samplingModeBinding.values = rayTracingSamplingModeValues;
        samplingModeBinding.enumType = RayTracingSamplingMode.class;

        TypedOption<Integer> rtsSamplesPerPixelOpt = new TypedOption<>(
            "-rts-samples-per-pixel",
            TypedOption.reference(() -> stochasticRayTracingState.samplesPerPixel, v -> stochasticRayTracingState.samplesPerPixel = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsNoProgressiveOpt = new TypedOption<>(
            "-rts-no-progressive",
            TypedOption.reference(() -> stochasticRayTracingState.progressiveTracing, v -> stochasticRayTracingState.progressiveTracing = v),
            0,
            OptionsGroupStochasticRaytracing::setIntFalse,
            null);
        TypedOption<EnumBinding<RayTracingRadMode>> rtsRadModeOpt = new TypedOption<>(
            "-rts-rad-mode",
            TypedOption.valueRef(radModeBinding),
            1,
            null,
            OptionsGroupStochasticRaytracing::parseEnumBinding);
        TypedOption<Integer> rtsNoLightSamplingOpt = new TypedOption<>(
            "-rts-no-lightsampling",
            TypedOption.reference(() -> stochasticRayTracingState.nextEvent, v -> stochasticRayTracingState.nextEvent = v),
            0,
            OptionsGroupStochasticRaytracing::setIntFalse,
            null);
        TypedOption<EnumBinding<RayTracingLightMode>> rtsLightModeOpt = new TypedOption<>(
            "-rts-l-mode",
            TypedOption.valueRef(lightModeBinding),
            1,
            null,
            OptionsGroupStochasticRaytracing::parseEnumBinding);
        TypedOption<Integer> rtsLightSamplesOpt = new TypedOption<>(
            "-rts-l-samples",
            TypedOption.reference(() -> stochasticRayTracingState.nextEventSamples, v -> stochasticRayTracingState.nextEventSamples = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsScatterSamplesOpt = new TypedOption<>(
            "-rts-scatter-samples",
            TypedOption.reference(() -> stochasticRayTracingState.scatterSamples, v -> stochasticRayTracingState.scatterSamples = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsDoFdgOpt = new TypedOption<>(
            "-rts-do-fdg",
            TypedOption.reference(() -> stochasticRayTracingState.differentFirstDG, v -> stochasticRayTracingState.differentFirstDG = v),
            0,
            OptionsGroupStochasticRaytracing::setIntTrue,
            null);
        TypedOption<Integer> rtsFdgSamplesOpt = new TypedOption<>(
            "-rts-fdg-samples",
            TypedOption.reference(() -> stochasticRayTracingState.firstDGSamples, v -> stochasticRayTracingState.firstDGSamples = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsSeparateSpecularOpt = new TypedOption<>(
            "-rts-separate-specular",
            TypedOption.reference(() -> stochasticRayTracingState.separateSpecular, v -> stochasticRayTracingState.separateSpecular = v),
            0,
            OptionsGroupStochasticRaytracing::setIntTrue,
            null);
        TypedOption<EnumBinding<RayTracingSamplingMode>> rtsSamplingModeOpt = new TypedOption<>(
            "-rts-s-mode",
            TypedOption.valueRef(samplingModeBinding),
            1,
            null,
            OptionsGroupStochasticRaytracing::parseEnumBinding);
        TypedOption<Integer> rtsMinPathLengthOpt = new TypedOption<>(
            "-rts-min-path-length",
            TypedOption.reference(() -> stochasticRayTracingState.minPathDepth, v -> stochasticRayTracingState.minPathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsMaxPathLengthOpt = new TypedOption<>(
            "-rts-max-path-length",
            TypedOption.reference(() -> stochasticRayTracingState.maxPathDepth, v -> stochasticRayTracingState.maxPathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> rtsNoDirectBackgroundOpt = new TypedOption<>(
            "-rts-NOdirect-background-rad",
            TypedOption.reference(() -> stochasticRayTracingState.backgroundDirect, v -> stochasticRayTracingState.backgroundDirect = v),
            0,
            OptionsGroupStochasticRaytracing::setIntFalse,
            null);
        TypedOption<Integer> rtsNoIndirectBackgroundOpt = new TypedOption<>(
            "-rts-NOindirect-background-rad",
            TypedOption.reference(() -> stochasticRayTracingState.backgroundIndirect, v -> stochasticRayTracingState.backgroundIndirect = v),
            0,
            OptionsGroupStochasticRaytracing::setIntFalse,
            null);
        OptionBase[] stochasticRatTracerOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(rtsSamplesPerPixelOpt, 7),
            TypedOption.REGISTER_OPTION(rtsNoProgressiveOpt, 9),
            TypedOption.REGISTER_OPTION(rtsRadModeOpt, 8),
            TypedOption.REGISTER_OPTION(rtsNoLightSamplingOpt, 9),
            TypedOption.REGISTER_OPTION(rtsLightModeOpt, 8),
            TypedOption.REGISTER_OPTION(rtsLightSamplesOpt, 8),
            TypedOption.REGISTER_OPTION(rtsScatterSamplesOpt, 7),
            TypedOption.REGISTER_OPTION(rtsDoFdgOpt, 0),
            TypedOption.REGISTER_OPTION(rtsFdgSamplesOpt, 8),
            TypedOption.REGISTER_OPTION(rtsSeparateSpecularOpt, 8),
            TypedOption.REGISTER_OPTION(rtsSamplingModeOpt, 9),
            TypedOption.REGISTER_OPTION(rtsMinPathLengthOpt, 8),
            TypedOption.REGISTER_OPTION(rtsMaxPathLengthOpt, 8),
            TypedOption.REGISTER_OPTION(rtsNoDirectBackgroundOpt, 8),
            TypedOption.REGISTER_OPTION(rtsNoIndirectBackgroundOpt, 8)
        };
        OptionGroup[] stochasticRaytracerGroups = new OptionGroup[] {
            new OptionGroup("stochasticRaytracer", stochasticRatTracerOptions, 15)
        };
        OptionParser.parse(argc, argv, stochasticRaytracerGroups, 1);
    }
}
