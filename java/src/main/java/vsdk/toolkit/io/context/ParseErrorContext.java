package vsdk.toolkit.io.context;

// Error codes
public final class ParseErrorContext {
    public static final int MGF_OK = 0; // normal return value
    public static final int MGF_ERROR_UNKNOWN_ENTITY = 1;
    public static final int MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS = 2;
    public static final int MGF_ERROR_ARGUMENT_TYPE = 3;
    public static final int MGF_ERROR_ILLEGAL_ARGUMENT_VALUE = 4;
    public static final int MGF_ERROR_UNDEFINED_REFERENCE = 5;
    public static final int MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE = 6;
    public static final int MGF_ERROR_IN_INCLUDED_FILE = 7;
    public static final int MGF_ERROR_OUT_OF_MEMORY = 8;
    public static final int MGF_ERROR_FILE_SEEK_ERROR = 9;
    public static final int MGF_ERROR_LINE_TOO_LONG = 11;
    public static final int MGF_ERROR_UNMATCHED_CONTEXT_CLOSE = 12;
    public static final int MGF_NUMBER_OF_ERRORS = 13;

    private ParseErrorContext() {
    }
}
