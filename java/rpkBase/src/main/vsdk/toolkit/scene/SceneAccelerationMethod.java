package vsdk.toolkit.scene;

/**
The spatial index behind the scene query (option -scene-acceleration):
- VOXEL_ACCELERATION: VITRAL's GridSpatialIndex, the port of RenderPark's
  voxel grid ([SNYD1987]; the original VoxelGrid was retired in step 6 of
  the VITRAL alignment plan);
- WALD2007_ACCELERATION: VITRAL's BinnedSahSpatialIndex (the default);
- BRUTE_FORCE_ACCELERATION: VITRAL's LinearSpatialIndex, every face tested.
Every method prunes only: the exact face test decides each hit.
*/
public enum SceneAccelerationMethod {
    VOXEL_ACCELERATION,
    WALD2007_ACCELERATION,
    BRUTE_FORCE_ACCELERATION
}
