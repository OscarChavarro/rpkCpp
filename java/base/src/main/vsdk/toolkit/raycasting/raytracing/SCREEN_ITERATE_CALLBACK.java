package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

@FunctionalInterface
public interface SCREEN_ITERATE_CALLBACK {
    ColorRgbMutable call(Camera camera, VoxelGrid sceneVoxelGrid, Background sceneBackground, int x, int y, Object data);
}
