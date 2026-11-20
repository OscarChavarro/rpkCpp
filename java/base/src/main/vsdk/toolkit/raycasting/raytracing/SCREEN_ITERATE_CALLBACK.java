package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.scene.VoxelGridWorkspace;

@FunctionalInterface
public interface SCREEN_ITERATE_CALLBACK {
    ColorRgbMutable call(Camera camera, VoxelGrid sceneVoxelGrid, VoxelGridWorkspace sceneVoxelGridWorkspace, Background sceneBackground, int x, int y, Object data);
}
