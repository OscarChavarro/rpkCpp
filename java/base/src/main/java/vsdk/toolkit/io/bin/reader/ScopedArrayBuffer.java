package vsdk.toolkit.io.bin.reader;

public class ScopedArrayBuffer<T> {
    private T value;

    public ScopedArrayBuffer() {
        this(null);
    }

    public ScopedArrayBuffer(T initialValue) {
        value = initialValue;
    }

    public void reset(T newValue) {
        value = newValue;
    }

    public T get() {
        return value;
    }
}
