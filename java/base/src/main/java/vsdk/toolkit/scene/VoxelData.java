package vsdk.toolkit.scene;

import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

class VoxelData {
    Patch patch;
    Geometry geometry;
    VoxelGrid voxelGrid;

    int flags; // Patch or geometry? last ray id, ...

    VoxelData(VoxelGrid data, int flags) {
        this.geometry = null;
        this.patch = null;
        this.voxelGrid = data;
        this.flags = flags;
    }

    VoxelData(Patch data, int flags) {
        this.geometry = null;
        this.patch = data;
        this.voxelGrid = null;
        this.flags = flags;
    }

    VoxelData(Geometry data, int flags) {
        this.geometry = data;
        this.patch = null;
        this.voxelGrid = null;
        this.flags = flags;
    }

    void updateRayId(final int id) {
        flags = (flags & ~VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK) | (id & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK);
    }

    int lastRayId() {
        return flags & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
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
