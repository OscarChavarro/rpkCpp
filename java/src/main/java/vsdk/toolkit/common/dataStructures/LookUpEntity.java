package vsdk.toolkit.common.dataStructures;

public class LookUpEntity<T> {
    public String key; // Key name
    public long value; // Key hash value (for efficiency)
    public T data; // Client data

    public LookUpEntity() {
        key = null;
        value = 0;
        data = null;
    }
}
