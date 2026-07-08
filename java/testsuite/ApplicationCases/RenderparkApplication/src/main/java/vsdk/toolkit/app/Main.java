package vsdk.toolkit.app;

import vsdk.toolkit.common.Random;

public class Main {
    public static void main(String[] args) {
        String[] argv = new String[args == null ? 1 : args.length + 1];
        argv[0] = "rpk";
        if ( args != null ) {
            System.arraycopy(args, 0, argv, 1, args.length);
        }

        // RpkApplication.entryPoint() starts Random once -nativeRNG has been
        // parsed. System.exit() below never returns, so a try/finally here
        // would not run: register a shutdown hook to guarantee
        // Random.shutdown() still executes, whichever way the JVM terminates.
        Runtime.getRuntime().addShutdownHook(new Thread(Random::shutdown));

        RpkApplication application = new RpkApplication();
        int result = application.entryPoint(argv.length, argv);
        System.exit(result);
    }
}
