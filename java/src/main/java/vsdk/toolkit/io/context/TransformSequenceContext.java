package vsdk.toolkit.io.context;

public class TransformSequenceContext {
    public static final int TRANSFORM_MAXIMUM_DIMENSIONS = 8;

    public FilePositionContext startingPosition; // Starting position on input
    public int numberOfDimensions; // Number of array dimensions
    public TransformArrayContext[] transformArguments;

    public TransformSequenceContext() {
        startingPosition = new FilePositionContext();
        numberOfDimensions = 0;
        transformArguments = new TransformArrayContext[TRANSFORM_MAXIMUM_DIMENSIONS];
        for (int i = 0; i < transformArguments.length; i++) {
            transformArguments[i] = new TransformArrayContext();
        }
    }
}
