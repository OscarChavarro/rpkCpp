package vsdk.toolkit.common.dataStructures;

/**
Node for a balanced kd tree, nodes are placed in arrays
and no loson, hison pointers are necessary
*/

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
