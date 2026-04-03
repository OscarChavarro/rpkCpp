package vsdk.toolkit.common.dataStructures;

public class KDTreeNode {
    public KDTreeNode loson;
    public KDTreeNode hison;

    public int mFlags;
    public Object mData;

    public int discriminator() {
        return mFlags & 0xF;
    }

    public void setDiscriminator(int discriminator) {
        mFlags = (mFlags & 0xFFF0) | discriminator;
    }

    public int flags() {
        return mFlags & 0xFFF0;
    }

    public void findMinMaxDepth(int depth, int[] minDepth, int[] maxDepth) {
        if ((loson == null) && (hison == null)) {
            maxDepth[0] = Math.max(maxDepth[0], depth);
            minDepth[0] = Math.min(minDepth[0], depth);
        }
        else {
            if (loson != null) {
                loson.findMinMaxDepth(depth + 1, minDepth, maxDepth);
            }
            if (hison != null) {
                hison.findMinMaxDepth(depth + 1, minDepth, maxDepth);
            }
        }
    }
}
