package vsdk.toolkit.app.options;

public final class OptionsGroupRaytracing {
    private static void makeMethodsHelpMessage(String[] buffer) {
        buffer[0] =
            "-raytracing-method <method>: set pixel-based radiance computation method\n"
            + "\tmethods: none                 no pixel-based radiance computation\n"
            + "\t         StochasticRaytracing Stochastic Raytracing & Final Gathers (default)\n"
            + "\t         BidirectionalPathTra Bidirectional Path Tracing\n"
            + "\t         RayCasting           Ray Casting\n"
            + "\t         RayMatting           Ray Matting";
    }

    public static void parse(
        int[] argc,
        String[] argv,
        String[] rayTracerName)
    {
        String[] helpMessage = new String[1];

        OptionsGroupRaytracing.makeMethodsHelpMessage(helpMessage);
        if (rayTracerName != null && rayTracerName.length > 0) {
            rayTracerName[0] = "none";
        }
        OptionsGroupRaytracingMethod.rayTracingParseOptions(argc, argv, helpMessage, rayTracerName);
    }
}
