package vsdk.toolkit.common.dataStructures;

public class CircularListNode<T> extends CircularListLink {
    public T data;

    public CircularListNode(T inData) {
        data = inData;
    }
}
