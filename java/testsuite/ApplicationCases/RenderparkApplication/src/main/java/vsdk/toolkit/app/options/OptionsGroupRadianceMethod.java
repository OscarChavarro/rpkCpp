package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;

public final class OptionsGroupRadianceMethod {
    public static void radianceMethodParseOptions(
        int[] argc,
        String[] argv,
        String[] radianceMethodsStringOut)
    {
        TypedOption.ValueRef<String> radianceMethodName = TypedOption.valueRef(null);
        TypedOption<String> radianceMethodOpt = new TypedOption<>(
            "-radiance-method",
            radianceMethodName,
            1,
            null,
            null);
        OptionBase[] radianceOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(radianceMethodOpt, 4)
        };
        OptionGroup[] radianceGroups = new OptionGroup[] {
            new OptionGroup("radiance", radianceOptions, 1)
        };
        OptionParser.parse(argc, argv, radianceGroups, 1);

        if (radianceMethodsStringOut != null && radianceMethodsStringOut.length > 0) {
            radianceMethodsStringOut[0] = radianceMethodName.get() != null ? radianceMethodName.get() : "";
        }
    }
}
