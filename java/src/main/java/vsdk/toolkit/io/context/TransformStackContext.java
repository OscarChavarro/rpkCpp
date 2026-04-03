package vsdk.toolkit.io.context;

public class TransformStackContext {
    public long xid; // Unique transform id
    public short xac; // Context argument count
    public short rev; // Boolean true if vertices reversed
    public short ownedArgumentCount; // Number of owned argument copies
    public TransformContext xf; // Cumulative transformation
    public TransformSequenceContext transformationArray;
    public String[] ownedArgumentCopies; // Copies for non-iterative transform arguments
    public TransformStackContext prev; // Previous transformation context

    public TransformStackContext() {
        xid = 0;
        xac = 0;
        rev = 0;
        ownedArgumentCount = 0;
        xf = new TransformContext();
        transformationArray = null;
        ownedArgumentCopies = null;
        prev = null;
    }
}
