package vsdk.toolkit.scene;

public final class VoxelDataFlags {
    public static final int VOXEL_DATA_PATCH_MASK = 0x10000000;
    public static final int VOXEL_DATA_GEOMETRY_MASK = 0x20000000;
    public static final int VOXEL_DATA_GRID_MASK = 0x40000000;
    public static final int VOXEL_DATA_RAY_COUNT_MASK = 0x0fffffff;

    private VoxelDataFlags() {
    }
}
