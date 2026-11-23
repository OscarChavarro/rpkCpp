package vsdk.toolkit.common.dataStructures;

public final class BalancedKDTreeNode<T> {
    public final T data;
    private int flags;

    public BalancedKDTreeNode(T data, int flags) {
        this.data = data;
        this.flags = flags;
    }
    public int discriminator() { return flags & 0xf; }
    public int flags() { return flags & 0xfff0; }
    public void setDiscriminator(int axis) { flags = (flags & 0xfff0) | axis; }
}
