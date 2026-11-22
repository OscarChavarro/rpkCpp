package vsdk.toolkit.common.dataStructures;

public class CircularListIterator<T> extends CircularListBaseIterator {
    public CircularListIterator(CircularList<T> list) {
        super(list.baseList());
    }

    public T nextOnSequence() {
        CircularListNode<T> link = (CircularListNode<T>)next();
        return link != null ? link.data : null;
    }

    public void init(CircularList<T> list) {
        super.init(list.baseList());
    }
}
