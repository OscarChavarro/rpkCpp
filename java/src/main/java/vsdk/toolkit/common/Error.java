package vsdk.toolkit.common;

/**
Prints an error message. Behaves much like printf. The first argument is the
name of the routine in which the error occurs (optional - can be nullptr)
*/

/**
Fatal error: print message or and exit the program with the specified error code
First argument is a return code. We use negative return codes for
"internal" error messages
*/

/**
Same, but for warning messages
*/

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
