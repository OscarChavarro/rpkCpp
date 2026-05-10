package vsdk.toolkit.common.memoryManagement;

import java.util.ArrayList;
import java.util.function.Supplier;

/**
 * Java equivalent of the legacy C++ template MemoryPool.
 * This pool keeps object instances and follows stack-like borrow/free usage.
 */
public final class MemoryPool<T> {
    private final Supplier<T> supplier;
    private final ArrayList<T> available;
    private final ArrayList<T> borrowedStack;
    private boolean initialized;

    public MemoryPool(Supplier<T> supplier) {
        this.supplier = supplier;
        this.available = new ArrayList<>();
        this.borrowedStack = new ArrayList<>();
        this.initialized = false;
    }

    public void init(long sizeInBytes) {
        if (initialized || sizeInBytes <= 0 || supplier == null) {
            return;
        }
        initialized = true;
    }

    public T allocate(int numberOfElements) {
        if (!initialized || numberOfElements <= 0) {
            return null;
        }
        if (available.isEmpty()) {
            return null;
        }
        T out = available.remove(available.size() - 1);
        borrowedStack.add(out);
        return out;
    }

    public void free(int numberOfElements) {
        if (!initialized || numberOfElements <= 0) {
            return;
        }
        if (borrowedStack.isEmpty()) {
            return;
        }
        T item = borrowedStack.remove(borrowedStack.size() - 1);
        available.add(item);
    }

    public void clear() {
        if (!initialized) {
            return;
        }
        while (!borrowedStack.isEmpty()) {
            available.add(borrowedStack.remove(borrowedStack.size() - 1));
        }
    }

    public boolean expand(int numberOfElements) {
        if (numberOfElements <= 0 || supplier == null) {
            return numberOfElements <= 0;
        }
        initialized = true;
        for (int i = 0; i < numberOfElements; i++) {
            available.add(supplier.get());
        }
        return true;
    }
}
