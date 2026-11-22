package vsdk.toolkit.scene;

/**
The spatial index behind the scene query (option -scene-acceleration):
- LEGACY_VOXEL_ACCELERATION: RenderPark's VoxelGrid, kept as a comparison
  until step 6 of the VITRAL alignment plan retires it;
- VOXEL_ACCELERATION: VITRAL's GridSpatialIndex, the port of that grid;
- WALD2007_ACCELERATION: VITRAL's BinnedSahSpatialIndex (the default);
- BRUTE_FORCE_ACCELERATION: VITRAL's LinearSpatialIndex, every face tested.
Every method prunes only: the exact face test decides each hit.
*/
public enum SceneAccelerationMethod {
    LEGACY_VOXEL_ACCELERATION,
    VOXEL_ACCELERATION,
    WALD2007_ACCELERATION,
    BRUTE_FORCE_ACCELERATION
}
