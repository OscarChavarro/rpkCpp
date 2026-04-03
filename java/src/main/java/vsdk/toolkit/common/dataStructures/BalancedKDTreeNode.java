package vsdk.toolkit.common.dataStructures;

public class BalancedKDTreeNode {
    public Object mData;
    public int mFlags;

    public void copy(KDTreeNode kdNode) {
        mData = kdNode.mData;
        mFlags = kdNode.flags();
    }

    public int discriminator() {
        return mFlags & 0xF;
    }

    public void setDiscriminator(int discr) {
        mFlags = (mFlags & 0xFFF0) | discr;
    }
}
