package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3D;

/**
Per-query scratch of a VoxelGrid traversal: the ray counter and, for every
item of the grid hierarchy, the counter of the last ray that tested it. Items
spanning several cells are so tested only once per ray.

Workspace: mutable, owned by one caller and passed explicitly on every
gridIntersect call. The grid itself is never written during a query.

Java adaptation: the traversal variables that the C++ port keeps on the stack
live here too, one frame per grid nesting depth, so a query allocates nothing.
*/
public class VoxelGridWorkspace {
    /**
    Traversal variables of one gridIntersect call at a given nesting depth
    */
    static final class TraversalFrame {
        final Vector3D tNext = new Vector3D();
        final Vector3D tDelta = new Vector3D();
        final Vector3D p = new Vector3D();
        final int[] step = new int[3];
        final int[] out = new int[3];
        final int[] g = new int[3];
        final float[] t0 = new float[1];
    }

    private final int[] lastRayIdByItem;
    private final TraversalFrame[] frames;
    private int rayCount;

    public VoxelGridWorkspace(int numberOfItems, int numberOfLevels) {
        lastRayIdByItem = new int[Math.max(numberOfItems, 1)];
        frames = new TraversalFrame[Math.max(numberOfLevels, 1)];
        for (int i = 0; i < frames.length; i++) {
            frames[i] = new TraversalFrame();
        }
        rayCount = 0;
    }

    int nextRayId() {
        rayCount++;
        return rayCount & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
    }

    int lastRayId(int itemIndex) {
        return lastRayIdByItem[itemIndex];
    }

    void updateRayId(int itemIndex, int id) {
        lastRayIdByItem[itemIndex] = id & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
    }

    TraversalFrame frame(int depth) {
        return frames[depth];
    }

    public int getNumberOfItems() {
        return lastRayIdByItem.length;
    }
}
