package vsdk.toolkit.common.commandLineOptions;

public class OptionParser<TOptionBase extends OptionBase> {
    public static <TOptionBase extends OptionBase> boolean parse(
        int[] argc,
        String[] argv,
        TOptionBase[] registry,
        int registryCount)
    {
        return parse(argc, argv, registry, registryCount, null);
    }

    @SuppressWarnings("unchecked")
    public static <TOptionBase extends OptionBase> boolean parse(
        int[] argc,
        String[] argv,
        TOptionBase[] registry,
        int registryCount,
        Object context)
    {
        OptionGroupT<TOptionBase> singleGroup = new OptionGroupT<>("default", registry, registryCount);
        OptionGroupT<TOptionBase>[] groups = (OptionGroupT<TOptionBase>[])new OptionGroupT[]{singleGroup};
        return parse(argc, argv, groups, 1, context);
    }

    public static <TOptionBase extends OptionBase> boolean parse(
        int[] argc,
        String[] argv,
        OptionGroupT<TOptionBase>[] groups,
        int groupCount)
    {
        return parse(argc, argv, groups, groupCount, null);
    }

    public static <TOptionBase extends OptionBase> boolean parse(
        int[] argc,
        String[] argv,
        OptionGroupT<TOptionBase>[] groups,
        int groupCount,
        Object context)
    {
        if (groups == null || groupCount <= 0 || argc == null || argc.length == 0 || argv == null) {
            return false;
        }

        for (int i = 0; i < argc[0]; i++) {
            if (argv[i] == null) {
                continue;
            }
            boolean matched = false;
            for (int g = 0; g < groupCount && !matched; g++) {
                if (groups[g].options == null || groups[g].count <= 0) {
                    continue;
                }
                for (int j = 0; j < groups[g].count; j++) {
                    if (!groups[g].options[j].isConfigured()) {
                        continue;
                    }
                    if (!TypedOption.matchOption(argv[i], groups[g].options[j].getName(), groups[g].options[j].getAbbreviationLength())) {
                        continue;
                    }

                    int consumesValue = groups[g].options[j].consumesValue();
                    if (consumesValue != 0) {
                        if (i + consumesValue >= argc[0]) {
                            return false;
                        }

                        boolean missingValue = false;
                        for (int k = 1; k <= consumesValue; k++) {
                            if (argv[i + k] == null) {
                                missingValue = true;
                                break;
                            }
                        }
                        if (missingValue) {
                            return false;
                        }

                        String[] argsSlice = new String[consumesValue];
                        System.arraycopy(argv, i + 1, argsSlice, 0, consumesValue);
                        if (!groups[g].options[j].apply(context, consumesValue, argsSlice)) {
                            return false;
                        }

                        argv[i] = null;
                        for (int k = 1; k <= consumesValue; k++) {
                            argv[i + k] = null;
                        }
                        i += consumesValue;
                    }
                    else {
                        if (!groups[g].options[j].apply(context, 0, null)) {
                            return false;
                        }
                        argv[i] = null;
                    }

                    matched = true;
                    break;
                }
            }
        }

        int writeIndex = 0;
        for (int readIndex = 0; readIndex < argc[0]; readIndex++) {
            if (argv[readIndex] != null) {
                argv[writeIndex++] = argv[readIndex];
            }
        }
        for (int i = writeIndex; i < argc[0]; i++) {
            argv[i] = null;
        }

        argc[0] = writeIndex;
        return true;
    }
}
