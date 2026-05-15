package vsdk.toolkit.io.context;

import java.io.InputStream;

public class ReaderContext {
    public static final int MGF_MAXIMUM_INPUT_LINE_LENGTH = 4096;
    public static final int MGF_MAXIMUM_ARGUMENT_COUNT = (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4);

    public String fileName;
    public InputStream inputStream; // stream pointer
    public int fileContextId;
    public String inputLine;
    public int lineNumber;
    public char isPipe; // Flag indicating whether input comes from a pipe or a real file
    public ReaderContext prev; // Previous context

    public ReaderContext() {
        fileName = "";
        inputStream = null;
        fileContextId = 0;
        inputLine = "";
        lineNumber = 0;
        isPipe = 0;
        prev = null;
    }
}
