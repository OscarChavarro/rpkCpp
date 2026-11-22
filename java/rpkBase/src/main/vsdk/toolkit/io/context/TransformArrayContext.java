package vsdk.toolkit.io.context;

public class TransformArrayContext {
    public short i; // Current count
    public short n; // Current maximum
    public String arg; // String argument value
    public short argumentIndex; // Index in active transform argument vector

    public TransformArrayContext() {
        arg = "";
        argumentIndex = -1;
    }
}
