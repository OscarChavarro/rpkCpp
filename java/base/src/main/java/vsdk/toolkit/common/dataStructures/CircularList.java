package vsdk.toolkit.common.dataStructures;

/**
Implementation is based on Stroustrup 'The C++ Programming Language' Section 8.3
*/

public class CircularList<T> extends CircularListBase {
    public void add(T data) {
        addLink(new CircularListNode<>(data));
    }

    public void append(T data) {
        appendLink(new CircularListNode<>(data));
    }

    public void removeAll() {
        CircularListNode<T> link = (CircularListNode<T>)remove();
        while (link != null) {
            link.nextLink = null;
            link = (CircularListNode<T>)remove();
        }
    }

    @Override
    public void clear() {
        super.clear();
    }

    public CircularListBase baseList() {
        return this;
    }
}
