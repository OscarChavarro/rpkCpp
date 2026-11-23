package vsdk.toolkit.common.dataStructures;

public final class KDTreeNode<T> {
    public KDTreeNode<T> loson;
    public KDTreeNode<T> hison;
    public int flags;
    public T data;

    public KDTreeNode(T data, int flags) {
        this.data = data;
        this.flags = flags & 0xfff0;
    }
    public int discriminator() { return flags & 0xf; }
    public void setDiscriminator(int axis) { flags = (flags & 0xfff0) | axis; }
}
