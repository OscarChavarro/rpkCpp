export class ParseErrorContext {
  public static readonly MGF_OK = 0;
  public static readonly MGF_ERROR_UNKNOWN_ENTITY = 1;
  public static readonly MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS = 2;
  public static readonly MGF_ERROR_ARGUMENT_TYPE = 3;
  public static readonly MGF_ERROR_ILLEGAL_ARGUMENT_VALUE = 4;
  public static readonly MGF_ERROR_UNDEFINED_REFERENCE = 5;
  public static readonly MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE = 6;
  public static readonly MGF_ERROR_IN_INCLUDED_FILE = 7;
  public static readonly MGF_ERROR_OUT_OF_MEMORY = 8;
  public static readonly MGF_ERROR_FILE_SEEK_ERROR = 9;
  public static readonly MGF_ERROR_LINE_TOO_LONG = 11;
  public static readonly MGF_ERROR_UNMATCHED_CONTEXT_CLOSE = 12;
  public static readonly MGF_NUMBER_OF_ERRORS = 13;

  private constructor() {
  }
}
