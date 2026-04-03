/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/

package vsdk.toolkit.raycasting.simple;

import java.util.ArrayList;
import java.util.concurrent.ThreadLocalRandom;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.raycasting.common.BoxFilter;
import vsdk.toolkit.raycasting.common.NormalFilter;
import vsdk.toolkit.raycasting.common.PixelFilter;
import vsdk.toolkit.raycasting.common.RayTools;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.raycasting.common.TentFilter;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class RayMatter extends RayTracer {
    private static RayMatter rayMatter = null;
    private static final String NAME = "Ray Matting";
    private ScreenBuffer screenBuffer;
    private PixelFilter pixelFilter;
    private boolean doDeleteScreen;
    private RayMatterState rayMatterState;

    public RayMatter(
        ScreenBuffer screen,
        Camera camera,
        RayMatterState inRayMatterState,
        ToneMappingContext toneMapOptions)
    {
        rayMatterState = inRayMatterState;

        if (screen == null) {
            screenBuffer = new ScreenBuffer(null, camera, toneMapOptions);
            doDeleteScreen = false;
        }
        else {
            screenBuffer = screen;
            screenBuffer.setToneMappingContext(toneMapOptions);
            doDeleteScreen = false;
        }

        pixelFilter = null;
        screenBuffer.setRgbImage(true);
    }

    @Override
    public void defaults() {
        // Defaults are owned by the caller-provided RayMatterState instance.
    }

    @Override
    public String getName() {
        return NAME;
    }

    @Override
    public void initialize(ArrayList<Patch> lightPatches) {
    }

    @Override
    public void
    execute(
        ImageOutputHandle ip,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if (rayMatter != null) {
            rayMatter = null;
        }
        rayMatter = new RayMatter(
            null,
            scene.camera,
            rayMatterState,
            toneMapOptions);
        rayMatter.doMatting(scene.camera, scene.voxelGrid);
        if (ip != null && rayMatter != null) {
            rayMatter.save(ip);
        }
    }

    @Override
    public boolean saveImage(ImageOutputHandle imageOutputHandle) {
        if (rayMatter == null) {
            return false;
        }

        rayMatter.save(imageOutputHandle);
        return true;
    }

    @Override
    public void terminate() {
        rayMatter = null;
    }

    public void createFilter() {
        if (pixelFilter != null) {
            pixelFilter = null;
        }

        if (rayMatterState.filter == RayMatterFilterType.BOX_FILTER) {
            pixelFilter = new BoxFilter();
        }
        if (rayMatterState.filter == RayMatterFilterType.TENT_FILTER) {
            pixelFilter = new TentFilter();
        }
        if (rayMatterState.filter == RayMatterFilterType.GAUSS_FILTER) {
            pixelFilter = new NormalFilter();
        }
        if (rayMatterState.filter == RayMatterFilterType.GAUSS2_FILTER) {
            pixelFilter = new NormalFilter(0.5, 1.5);
        }
    }

    public void doMatting(Camera camera, VoxelGrid sceneWorldVoxelGrid) {
        long t = System.nanoTime();

        createFilter();

        // Main loop for ray matter
        for (int y = 0; y < camera.ySize; y++) {
            for (int x = 0; x < camera.xSize; x++) {
                float hits = 0;

                for (int i = 0; i < rayMatterState.samplesPerPixel; i++) {
                    // Uniform random var
                    double[] dx = new double[] {ThreadLocalRandom.current().nextDouble()};
                    double[] dy = new double[] {ThreadLocalRandom.current().nextDouble()};

                    // Insert non-uniform sampling here
                    if (pixelFilter != null) {
                        pixelFilter.sample(dx, dy);
                    }

                    // Generate ray
                    Ray ray = new Ray();
                    ray.position = camera.eyePosition;
                    ray.direction = screenBuffer.getPixelVector(x, y, (float)dx[0], (float)dy[0]);
                    ray.direction.normalize(Numeric.EPSILON_FLOAT);

                    // Check if hit
                    if (RayTools.findRayIntersection(sceneWorldVoxelGrid, ray, null, null, null) != null) {
                        hits++;
                    }
                }

                // Add matte value to screen buffer
                float value = (hits / (float)rayMatterState.samplesPerPixel);
                if (value > 1.0f) {
                    value = 1.0f;
                }

                ColorRgb matte = new ColorRgb();
                matte.set(value, value, value);
                screenBuffer.add(x, y, matte);
            }

            screenBuffer.renderScanline(y);
        }

        Statistics.instance().rayTracer.totalTime = (double)(System.nanoTime() - t) / 1000000000.0;
        Statistics.instance().rayTracer.rayCount = 0;
        Statistics.instance().rayTracer.pixelCount = 0;
    }

    public void display() {
        screenBuffer.render();
    }

    public void save(ImageOutputHandle ip) {
        screenBuffer.writeFile(ip);
    }
}
