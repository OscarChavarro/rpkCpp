package vsdk.toolkit.common;

public final class Error {
    private Error() {
    }

    public static void error(String routine, String text, Object... args) {
        System.err.print("Error: ");
        if (routine != null) {
            System.err.print(routine + "(): ");
        }
        System.err.print(formatMessage(text, args));
        System.err.print(".\n");
        System.err.flush();
    }

    public static void warning(String routine, String text, Object... args) {
        System.err.print("Warning: ");
        if (routine != null) {
            System.err.print(routine + "(): ");
        }
        System.err.print(formatMessage(text, args));
        System.err.print(".\n");
        System.err.flush();
    }

    public static void fatal(int errcode, String routine, String text, Object... args) {
        System.err.print("logFatal error: ");
        if (routine != null) {
            System.err.print(routine + "(): ");
        }
        System.err.print(formatMessage(text, args));
        System.err.print(".\n");
        System.err.flush();

        System.exit(errcode);
    }

    private static String formatMessage(String text, Object... args) {
        if (text == null) {
            return "";
        }
        try {
            return String.format(text, args);
        }
        catch (Exception e) {
            return text;
        }
    }
}
