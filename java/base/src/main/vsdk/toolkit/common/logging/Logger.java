package vsdk.toolkit.common.logging;

import vsdk.toolkit.common.VSDKFatalException;

/**
Logger class centralizes error and exception reporting utilities.
*/
public final class Logger
{
    public static final int WARNING = 1;
    public static final int ERROR = 2;
    public static final int FATAL_ERROR = 3;

    private static boolean withSystemExit;
    private static boolean withFatalExceptions;

    static {
        // The system property lets a test JVM turn process termination off
        // once for every class (-Dvsdk.withSystemExit=false), instead of each
        // test class toggling it.
        withSystemExit = !"false".equalsIgnoreCase(
            System.getProperty("vsdk.withSystemExit", "true"));
        withFatalExceptions = true;
    }

    public static boolean isWithSystemExit()
    {
        return withSystemExit;
    }

    private Logger()
    {
    }

    /**
    A fatal error never terminates the process: it is raised to the caller as
    an unchecked VSDKFatalException, and the calling application decides how
    to handle it. `withSystemExit` (the former process-termination switch) and
    `withFatalExceptions` both ask for that exception; with both off, the
    report is only logged.
    */
    private static void processFatalError(String method, String message, Exception cause)
    {
        if ( !withSystemExit && !withFatalExceptions ) {
            return;
        }

        String m;
        if ( method == null || method.length() == 0 ) {
            m = "VSDK fatal error";
        }
        else {
            m = "VSDK fatal error at " + method;
        }
        if ( message != null && message.length() > 0 ) {
            m = m + ": " + message;
        }

        if ( cause != null ) {
            throw new VSDKFatalException(m, cause);
        }
        throw new VSDKFatalException(m);
    }

    public static void reportMessageWithException(Object o, int level, String method, String message, Exception ee)
    {
        String msg;
        int i;
        StackTraceElement report[];

        msg = "===========================================================================\n";
        msg = msg + "= VSDK Exception report                                                   =\n";
        if ( o != null ) {
            msg = msg + " - An exception has been thrown in the \"" + o.getClass().getName() + "\" class\n";
        }
        else {
            msg = msg + " - An exception has been thrown from a static context\n";
        }
        msg = msg + " - Exception located at method " + method + "\n";
        msg = msg + " - Vitral exception message:\n" + message + "\n";

        if ( ee != null ) {
            msg = msg + " - Java exception class:\n" + ee.getClass().getName() + "\n";
            msg = msg + " - Java exception message:\n" + ee.getMessage() + "\n";
            report = ee.getStackTrace();

            for ( i = 0; i < report.length; i++ ) {
                msg = msg + report[i] + "\n";
            }
        }
        else {
            msg = msg + " - Java exception is null! No detailed information about error.\n";
        }
        msg = msg + "===========================================================================\n";
        if ( level == FATAL_ERROR ) {
            msg = msg + "Program excecution suspended!\n";
        }

        System.err.println(msg);

        System.err.println("---------------------------------------------------------------------------");
        if ( ee != null ) {
            System.err.println(ee.getMessage());
            report = ee.getStackTrace();
            for ( i = 0; i < report.length; i++ ) {
                System.err.println(report[i]);
            }
        }
        else {
            System.err.println("Given exception is null! not reporting details!");
        }
        System.err.println("---------------------------------------------------------------------------");

        if ( level == FATAL_ERROR ) {
            try {
                throw new Exception("Logger.reportMessage(FATAL_ERROR)");
            }
            catch ( Exception e ) {
                System.err.println(e.getMessage());
                report = e.getStackTrace();
                for ( i = 0; i < report.length; i++ ) {
                    System.err.println(report[i]);
                }
            }

            processFatalError(method, message, ee);
        }
    }

    public static void reportMessage(Object o, int level, String method, String message)
    {
        String msg;

        msg = "===========================================================================\n";
        msg = msg + "= VSDK Exception report                                                   =\n";
        if ( o != null ) {
            msg = msg + " - An exception has been thrown in the \"" + o.getClass().getName() + "\" class\n";
        }
        else {
            msg = msg + " - An exception has been thrown from a static context\n";
        }
        msg = msg + " - Exception located at method " + method + "\n";
        msg = msg + " - Exception message:\n" + message + "\n";
        msg = msg + "===========================================================================\n";
        if ( level == FATAL_ERROR ) {
            msg = msg + "Program excecution suspended!\n";
        }

        System.err.println(msg);

        if ( level == FATAL_ERROR ) {
            try {
                throw new Exception("Logger.reportMessage(FATAL_ERROR)");
            }
            catch ( Exception e ) {
                System.err.println(e.getMessage());
                StackTraceElement report[];
                report = e.getStackTrace();
                int i;
                for ( i = 0; i < report.length; i++ ) {
                    System.err.println(report[i]);
                }
            }
            processFatalError(method, message, null);
        }
    }

    public static void setWithSystemExit(boolean flag)
    {
        withSystemExit = flag;
    }

    public static void setWithFatalExceptions(boolean flag)
    {
        withFatalExceptions = flag;
    }
}
