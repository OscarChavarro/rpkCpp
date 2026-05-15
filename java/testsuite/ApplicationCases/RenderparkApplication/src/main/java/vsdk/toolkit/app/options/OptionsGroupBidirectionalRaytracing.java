package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathRaytracerConfig;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathTracingState;

public final class OptionsGroupBidirectionalRaytracing {
    private static final class FixedStringBinding {
        public TypedOption.Reference<String> target;
        public int maxLength;
    }

    private static int regExpStringLength = BidirectionalPathRaytracerConfig.MAX_REGEXP_SIZE;

    private static boolean parseFixedStringBinding(
        int argc,
        String[] argv,
        TypedOption.MutableValue<FixedStringBinding> bindingValue)
    {
        FixedStringBinding binding = bindingValue.value;
        if (argc < 1 || argv == null || argv[0] == null || binding == null || binding.target == null || binding.maxLength <= 0) {
            return false;
        }
        String value = argv[0];
        int maxTextLength = binding.maxLength - 1;
        if (maxTextLength < 0) {
            maxTextLength = 0;
        }
        if (value.length() > maxTextLength) {
            value = value.substring(0, maxTextLength);
        }
        binding.target.set(value);
        return true;
    }

    private static boolean parseBoolInt(
        int argc,
        String[] argv,
        TypedOption.MutableValue<Integer> value)
    {
        if (argc < 1 || argv == null || argv[0] == null) {
            return false;
        }
        OptionTextUtils.TypedIntValue parsed = new OptionTextUtils.TypedIntValue(value.value);
        if (!OptionTextUtils.parseBoolInt(argv[0], parsed)) {
            return false;
        }
        value.value = parsed.value;
        return true;
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
        BidirectionalPathTracingState bidirectionalPathState)
    {
        FixedStringBinding leBinding = new FixedStringBinding();
        leBinding.target = TypedOption.reference(() -> bidirectionalPathState.baseConfig.leRegExp, v -> bidirectionalPathState.baseConfig.leRegExp = v);
        leBinding.maxLength = regExpStringLength;
        FixedStringBinding ldBinding = new FixedStringBinding();
        ldBinding.target = TypedOption.reference(() -> bidirectionalPathState.baseConfig.ldRegExp, v -> bidirectionalPathState.baseConfig.ldRegExp = v);
        ldBinding.maxLength = regExpStringLength;
        FixedStringBinding liBinding = new FixedStringBinding();
        liBinding.target = TypedOption.reference(() -> bidirectionalPathState.baseConfig.liRegExp, v -> bidirectionalPathState.baseConfig.liRegExp = v);
        liBinding.maxLength = regExpStringLength;

        TypedOption<Integer> bidirSamplesPerPixelOpt = new TypedOption<>(
            "-bidir-samples-per-pixel",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.samplesPerPixel, v -> bidirectionalPathState.baseConfig.samplesPerPixel = v),
            1,
            null,
            null);
        TypedOption<Integer> bidirNoProgressiveOpt = new TypedOption<>(
            "-bidir-no-progressive",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.progressiveTracing, v -> bidirectionalPathState.baseConfig.progressiveTracing = v),
            0,
            OptionsGroupBidirectionalRaytracing::setIntFalse,
            null);
        TypedOption<Integer> bidirMaxEyePathLengthOpt = new TypedOption<>(
            "-bidir-max-eye-path-length",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.maximumEyePathDepth, v -> bidirectionalPathState.baseConfig.maximumEyePathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> bidirMaxLightPathLengthOpt = new TypedOption<>(
            "-bidir-max-light-path-length",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.maximumLightPathDepth, v -> bidirectionalPathState.baseConfig.maximumLightPathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> bidirMaxPathLengthOpt = new TypedOption<>(
            "-bidir-max-path-length",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.maximumPathDepth, v -> bidirectionalPathState.baseConfig.maximumPathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> bidirMinPathLengthOpt = new TypedOption<>(
            "-bidir-min-path-length",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.minimumPathDepth, v -> bidirectionalPathState.baseConfig.minimumPathDepth = v),
            1,
            null,
            null);
        TypedOption<Integer> bidirNoLightImportanceOpt = new TypedOption<>(
            "-bidir-no-light-importance",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.sampleImportantLights, v -> bidirectionalPathState.baseConfig.sampleImportantLights = v),
            0,
            OptionsGroupBidirectionalRaytracing::setIntFalse,
            null);
        TypedOption<Integer> bidirUseRegexpOpt = new TypedOption<>(
            "-bidir-use-regexp",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.useSpars, v -> bidirectionalPathState.baseConfig.useSpars = v),
            0,
            OptionsGroupBidirectionalRaytracing::setIntTrue,
            null);
        TypedOption<Integer> bidirUseEmittedOpt = new TypedOption<>(
            "-bidir-use-emitted",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.doLe, v -> bidirectionalPathState.baseConfig.doLe = v),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseBoolInt);
        TypedOption<FixedStringBinding> bidirRexpEmittedOpt = new TypedOption<>(
            "-bidir-rexp-emitted",
            TypedOption.valueRef(leBinding),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseFixedStringBinding);
        TypedOption<Integer> bidirRegDirectOpt = new TypedOption<>(
            "-bidir-reg-direct",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.doLD, v -> bidirectionalPathState.baseConfig.doLD = v),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseBoolInt);
        TypedOption<FixedStringBinding> bidirRexpDirectOpt = new TypedOption<>(
            "-bidir-rexp-direct",
            TypedOption.valueRef(ldBinding),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseFixedStringBinding);
        TypedOption<Integer> bidirRegIndirectOpt = new TypedOption<>(
            "-bidir-reg-indirect",
            TypedOption.reference(() -> bidirectionalPathState.baseConfig.doLI, v -> bidirectionalPathState.baseConfig.doLI = v),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseBoolInt);
        TypedOption<FixedStringBinding> bidirRexpIndirectOpt = new TypedOption<>(
            "-bidir-rexp-indirect",
            TypedOption.valueRef(liBinding),
            1,
            null,
            OptionsGroupBidirectionalRaytracing::parseFixedStringBinding);
        OptionBase[] bidirectionalOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(bidirSamplesPerPixelOpt, 8),
            TypedOption.REGISTER_OPTION(bidirNoProgressiveOpt, 11),
            TypedOption.REGISTER_OPTION(bidirMaxEyePathLengthOpt, 12),
            TypedOption.REGISTER_OPTION(bidirMaxLightPathLengthOpt, 12),
            TypedOption.REGISTER_OPTION(bidirMaxPathLengthOpt, 12),
            TypedOption.REGISTER_OPTION(bidirMinPathLengthOpt, 12),
            TypedOption.REGISTER_OPTION(bidirNoLightImportanceOpt, 11),
            TypedOption.REGISTER_OPTION(bidirUseRegexpOpt, 12),
            TypedOption.REGISTER_OPTION(bidirUseEmittedOpt, 12),
            TypedOption.REGISTER_OPTION(bidirRexpEmittedOpt, 13),
            TypedOption.REGISTER_OPTION(bidirRegDirectOpt, 12),
            TypedOption.REGISTER_OPTION(bidirRexpDirectOpt, 13),
            TypedOption.REGISTER_OPTION(bidirRegIndirectOpt, 12),
            TypedOption.REGISTER_OPTION(bidirRexpIndirectOpt, 13)
        };
        OptionGroup[] bidirectionalGroups = new OptionGroup[] {
            new OptionGroup("bidirectional", bidirectionalOptions, 14)
        };
        OptionParser.parse(argc, argv, bidirectionalGroups, 1);
    }
}
