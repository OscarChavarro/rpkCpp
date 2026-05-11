/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/
package vsdk.toolkit.raycasting.simple;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.render.SoftIdsWrapper;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class RayCaster extends RayTracer {
    private static RayCaster rayCaster = null;
    private static final String NAME = "Ray Casting";
    private ScreenBuffer screenBuffer;
    private boolean doDeleteScreen;

    public RayCaster(ScreenBuffer inScreen, Camera defaultCamera, ToneMappingContext toneMapOptions) {
        if (inScreen == null) {
            screenBuffer = new ScreenBuffer(null, defaultCamera, toneMapOptions);
            doDeleteScreen = true;
        }
        else {
            screenBuffer = inScreen;
            screenBuffer.setToneMappingContext(toneMapOptions);
            doDeleteScreen = true;
        }
    }

    @Override
    public void defaults() {
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
        if (rayCaster != null) {
            rayCaster = null;
        }
        rayCaster = new RayCaster(null, scene.camera, toneMapOptions);
        rayCaster.render(scene, radianceMethod, toneMapOptions, renderOptions);
        if (rayCaster != null && ip != null) {
            rayCaster.save(ip);
        }
    }

    @Override
    public boolean saveImage(ImageOutputHandle imageOutputHandle) {
        if (rayCaster == null) {
            return false;
        }

        rayCaster.save(imageOutputHandle);
        return true;
    }

    @Override
    public void terminate() {
        rayCaster = null;
    }

    private static void clipUv(int numberOfVertices, double[] u, double[] v) {
        if (u[0] > 1.0 - Numeric.EPSILON) {
            u[0] = 1.0 - Numeric.EPSILON;
        }
        if (v[0] > 1.0 - Numeric.EPSILON) {
            v[0] = 1.0 - Numeric.EPSILON;
        }
        if (numberOfVertices == 3 && (u[0] + v[0]) > 1.0 - Numeric.EPSILON) {
            if (u[0] > v[0]) {
                u[0] = 1.0 - v[0] - Numeric.EPSILON;
            }
            else {
                v[0] = 1.0 - u[0] - Numeric.EPSILON;
            }
        }
        if (u[0] < Numeric.EPSILON) {
            u[0] = Numeric.EPSILON;
        }
        if (v[0] < Numeric.EPSILON) {
            v[0] = Numeric.EPSILON;
        }
    }

    /**
Determines the radiance of the nearest patch visible through the pixel
(x,y). P shall be the nearest patch visible in the pixel.
*/
    private ColorRgb
    getRadianceAtPixel(
        Camera camera,
        int x,
        int y,
        Patch patch,
        RadianceMethod radianceMethod,
        RenderOptions renderOptions)
    {
        ColorRgb radiance = new ColorRgb();
        radiance.clear();

        if (radianceMethod != null) {
            // Ray pointing from the eye through the center of the pixel.
            Ray ray = new Ray();
            ray.position = camera.eyePosition;
            ray.direction = screenBuffer.getPixelVector(x, y);
            ray.direction.normalize(Numeric.EPSILON_FLOAT);

            // Find intersection point of ray with patch
            Vector3D point = new Vector3D();
            float dist = patch.normal.dotProduct(ray.direction);
            dist = -(patch.normal.dotProduct(ray.position) + patch.planeConstant) / dist;
            point.sumScaled(ray.position, dist, ray.direction);

            // Find surface coordinates of hit point on patch
            double[] u = new double[1];
            double[] v = new double[1];
            patch.uv(point, u, v);

            // Boundary check is necessary because Z-buffer algorithm does
            // not yield exactly the same result as ray tracing at patch
            // boundaries.
            clipUv(patch.numberOfVertices, u, v);

            // Reverse ray direction and get radiance emitted at hit point towards the eye
            Vector3D dir = new Vector3D(-ray.direction.x, -ray.direction.y, -ray.direction.z);
            radiance = radianceMethod.getRadiance(camera, patch, u[0], v[0], dir, renderOptions);
        }
        return radiance;
    }

    public void
    render(
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        screenBuffer.setToneMappingContext(toneMapOptions);
        long t = System.nanoTime();

        SoftIdsWrapper idRenderer = new SoftIdsWrapper(scene, renderOptions);

        long[] width = new long[1];
        long[] height = new long[1];
        idRenderer.getSize(width, height);
        if (width[0] != screenBuffer.getHRes() || height[0] != screenBuffer.getVRes()) {
            Logger.fatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
        }

        // This is the main loop for ray-casting
        for (int y = 0; y < height[0]; y++) {
            for (int x = 0; x < width[0]; x++) {
                Patch patch = idRenderer.getPatchAtPixel(x, y);
                if (patch != null) {
                    ColorRgb rad = getRadianceAtPixel(scene.camera, x, y, patch, radianceMethod, renderOptions);
                    screenBuffer.add(x, y, rad);
                }
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
