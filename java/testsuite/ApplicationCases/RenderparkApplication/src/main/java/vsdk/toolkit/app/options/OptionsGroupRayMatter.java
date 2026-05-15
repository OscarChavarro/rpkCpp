package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.simple.RayMatterFilterType;
import vsdk.toolkit.raycasting.simple.RayMatterState;

public final class OptionsGroupRayMatter {
    private static final class EnumBinding<T extends Enum<T>> {
        public TypedOption.Reference<T> target;
        public EnumDesc[] values;
        public Class<T> enumType;
    }

    private static EnumDesc[] rayMatterPixelFilterValues = new EnumDesc[] {
        new EnumDesc(RayMatterFilterType.BOX_FILTER.ordinal(), "box", 2),
        new EnumDesc(RayMatterFilterType.TENT_FILTER.ordinal(), "tent", 2),
        new EnumDesc(RayMatterFilterType.GAUSS_FILTER.ordinal(), "gaussian 1/sqrt2", 2),
        new EnumDesc(RayMatterFilterType.GAUSS2_FILTER.ordinal(), "gaussian 1/2", 2),
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

    public static void rayMattingParseOptions(
        int[] argc,
        String[] argv,
        RayMatterState rayMatterState)
    {
        EnumBinding<RayMatterFilterType> pixelFilterBinding = new EnumBinding<>();
        pixelFilterBinding.target = TypedOption.reference(() -> rayMatterState.filter, v -> rayMatterState.filter = v);
        pixelFilterBinding.values = rayMatterPixelFilterValues;
        pixelFilterBinding.enumType = RayMatterFilterType.class;

        TypedOption<Integer> rmSamplesOpt = new TypedOption<>(
            "-rm-samples-per-pixel",
            TypedOption.reference(() -> rayMatterState.samplesPerPixel, v -> rayMatterState.samplesPerPixel = v),
            1,
            null,
            null);
        TypedOption<EnumBinding<RayMatterFilterType>> rmPixelFilterOpt = new TypedOption<>(
            "-rm-pixel-filter",
            TypedOption.valueRef(pixelFilterBinding),
            1,
            null,
            OptionsGroupRayMatter::parseEnumBinding);
        OptionBase[] rayMatterOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(rmSamplesOpt, 6),
            TypedOption.REGISTER_OPTION(rmPixelFilterOpt, 7)
        };

        OptionGroup[] rayMatterGroups = new OptionGroup[] {
            new OptionGroup("rayMatter", rayMatterOptions, 2)
        };
        OptionParser.parse(argc, argv, rayMatterGroups, 1);
    }
}
