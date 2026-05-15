package vsdk.toolkit.app;

public class Main {
    public static void main(String[] args) {
        String[] argv = new String[args == null ? 1 : args.length + 1];
        argv[0] = "rpk";
        if ( args != null ) {
            System.arraycopy(args, 0, argv, 1, args.length);
        }

        RpkApplication application = new RpkApplication();
        int result = application.entryPoint(argv.length, argv);
        System.exit(result);
    }
}
