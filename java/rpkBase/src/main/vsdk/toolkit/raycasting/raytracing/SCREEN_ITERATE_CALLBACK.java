package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;

@FunctionalInterface
public interface SCREEN_ITERATE_CALLBACK<T> {
    ColorRgb call(Camera camera, SceneQuery sceneQuery, SceneQueryWorkspace sceneSceneQueryWorkspace, Background sceneBackground, int x, int y, T data);
}
