package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;

public final class OptionsGroupRaytracingMethod {
    private static String raytracingMethodsString = null;
    private static String[] rayTracerName = null;

    private static void mainRayTracingOption(TypedOption.MutableValue<String> name) {
        if (rayTracerName != null && rayTracerName.length > 0) {
            rayTracerName[0] = name.value;
        }
    }

    public static void rayTracingParseOptions(
        int[] argc,
        String[] argv,
        String[] raytracingMethodsStringOut,
        String[] rayTracerNameOut)
    {
        TypedOption.ValueRef<String> raytracingMethodName = TypedOption.valueRef(null);
        TypedOption<String> raytracingMethodOpt = new TypedOption<>(
            "-raytracing-method",
            raytracingMethodName,
            1,
            OptionsGroupRaytracingMethod::mainRayTracingOption,
            null);
        OptionBase[] raytracingOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(raytracingMethodOpt, 4)
        };

        OptionsGroupRaytracingMethod.rayTracerName = rayTracerNameOut;
        OptionsGroupRaytracingMethod.raytracingMethodsString =
            raytracingMethodsStringOut != null && raytracingMethodsStringOut.length > 0
            ? raytracingMethodsStringOut[0]
            : null;
        OptionGroup[] raytracingGroups = new OptionGroup[] {
            new OptionGroup("raytracing", raytracingOptions, 1)
        };
        OptionParser.parse(argc, argv, raytracingGroups, 1);
    }
}
