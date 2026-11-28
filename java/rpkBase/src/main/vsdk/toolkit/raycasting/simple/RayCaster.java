/**
Ray casting using the polygon visibility buffer for rendering Patch pointers into
a software frame buffer directly.
*/
package vsdk.toolkit.raycasting.simple;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.render.ImageOutputTarget;
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
        ImageOutputTarget ip,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RendererConfiguration renderOptions)
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
    public boolean saveImage(ImageOutputTarget imageOutputTarget) {
        if (rayCaster == null) {
            return false;
        }

        rayCaster.save(imageOutputTarget);
        return true;
    }

    @Override
    public void terminate() {
        rayCaster = null;
    }

    private static void clipUv(int numberOfVertices, double[] u, double[] v) {
        if (u[0] > 1.0 - VSDK.EPSILON) {
            u[0] = 1.0 - VSDK.EPSILON;
        }
        if (v[0] > 1.0 - VSDK.EPSILON) {
            v[0] = 1.0 - VSDK.EPSILON;
        }
        if (numberOfVertices == 3 && (u[0] + v[0]) > 1.0 - VSDK.EPSILON) {
            if (u[0] > v[0]) {
                u[0] = 1.0 - v[0] - VSDK.EPSILON;
            }
            else {
                v[0] = 1.0 - u[0] - VSDK.EPSILON;
            }
        }
        if (u[0] < VSDK.EPSILON) {
            u[0] = VSDK.EPSILON;
        }
        if (v[0] < VSDK.EPSILON) {
            v[0] = VSDK.EPSILON;
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
        RendererConfiguration renderOptions)
    {
        ColorRgb radiance = new ColorRgb();
        radiance = new ColorRgb(0.0, 0.0, 0.0);

        if (radianceMethod != null) {
            // Ray pointing from the eye through the center of the pixel.
            Ray ray = new Ray();
            ray = ray.withOrigin(camera.eyePosition);
            ray = ray.withDirection(screenBuffer.getPixelVector(x, y));
            ray = ray.withDirection(ray.getDirection().normalized(Numeric.EPSILON));

            // Find intersection point of ray with patch
            Vector3Dd point = new Vector3Dd();
            double dist = patch.getNormal().dotProduct(ray.getDirection());
            dist = -(patch.getNormal().dotProduct(ray.getOrigin()) + patch.getPlaneConstant()) / dist;
            point = ray.getOrigin().add(ray.getDirection().multiply(dist));

            // Find surface coordinates of hit point on patch
            double[] u = new double[1];
            double[] v = new double[1];
            patch.uv(point, u, v);

            // Boundary check is necessary because Z-buffer algorithm does
            // not yield exactly the same result as ray tracing at patch
            // boundaries.
            clipUv(patch.getNumberOfVertices(), u, v);

            // Reverse ray direction and get radiance emitted at hit point towards the eye
            Vector3Dd dir = new Vector3Dd(-ray.getDirection().x(), -ray.getDirection().y(), -ray.getDirection().z());
            radiance = radianceMethod.getRadiance(camera, patch, u[0], v[0], dir, renderOptions);
        }
        return radiance;
    }

    public void
    render(
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RendererConfiguration renderOptions)
    {
        screenBuffer.setToneMappingContext(toneMapOptions);
        long t = System.nanoTime();

        SoftIdsWrapper idRenderer = new SoftIdsWrapper(scene, renderOptions);

        long[] width = new long[1];
        long[] height = new long[1];
        idRenderer.getSize(width, height);
        if (width[0] != screenBuffer.getHRes() || height[0] != screenBuffer.getVRes()) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "RayCaster::render", "ID buffer size doesn't match screen size");
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

    public void save(ImageOutputTarget ip) {
        screenBuffer.writeFile(ip);
    }
}
