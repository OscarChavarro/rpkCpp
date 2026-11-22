package vsdk.toolkit.scene;

import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
One item stored in the cells of a VoxelGrid: a patch, a geometry or a sub
grid. The index is dense over the whole grid hierarchy and addresses the
per-item scratch of a VoxelGridWorkspace.

Shared, read-only after the grid is built.
*/
class VoxelData {
    private final Patch patch;
    private final Geometry geometry;
    private final VoxelGrid voxelGrid;
    private final int flags; // Patch, geometry or grid
    private final int index;

    VoxelData(VoxelGrid data, int flags, int index) {
        this.geometry = null;
        this.patch = null;
        this.voxelGrid = data;
        this.flags = flags;
        this.index = index;
    }

    VoxelData(Patch data, int flags, int index) {
        this.geometry = null;
        this.patch = data;
        this.voxelGrid = null;
        this.flags = flags;
        this.index = index;
    }

    VoxelData(Geometry data, int flags, int index) {
        this.geometry = data;
        this.patch = null;
        this.voxelGrid = null;
        this.flags = flags;
        this.index = index;
    }

    Patch getPatch() {
        return patch;
    }

    Geometry getGeometry() {
        return geometry;
    }

    VoxelGrid getVoxelGrid() {
        return voxelGrid;
    }

    int getIndex() {
        return index;
    }

    boolean isPatch() {
        return (flags & VoxelDataFlags.VOXEL_DATA_PATCH_MASK) != 0;
    }

    boolean isGeom() {
        return (flags & VoxelDataFlags.VOXEL_DATA_GEOMETRY_MASK) != 0;
    }

    boolean isGrid() {
        return (flags & VoxelDataFlags.VOXEL_DATA_GRID_MASK) != 0;
    }
}
