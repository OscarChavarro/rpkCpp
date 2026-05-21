/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.render.SoftIds;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class ScreenIterate {
    private static ScreenIterateState state = new ScreenIterateState();

    private static byte wakeUpRender() {
        return (byte)(1 << 1);
    }

    /**
    For counting how much CPU time was used for the computations
    */
    private static void updateCpuSecs() {
        final long now = System.nanoTime();
        Statistics.instance().rayTracer.totalTime += (double)(now - state.lastTime) / 1000000000.0;
        state.lastTime = now;
    }

    // ScreenIterateInit : initialise statistics and timers
    private static void init() {
        state.wakeUp = 0;

        // initialize for statistics etc.
        state.lastTime = System.nanoTime();
        Statistics.instance().rayTracer.resetCounters();
    }

    private static void finish() {
        updateCpuSecs();
    }

    public static void
    sequential(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SCREEN_ITERATE_CALLBACK callback,
        Object data,
        ToneMappingContext toneMapOptions)
    {
        int width;
        int height;
        ColorRgb col;
        ColorRgb[] rgb;

        init();

        width = camera.xSize;
        height = camera.ySize;
        rgb = new ColorRgb[width];
        for ( int i = 0; i < width; i++ ) {
            rgb[i] = new ColorRgb();
        }

        // Shoot rays through all the pixels
        for ( int i = 0; i < height; i++ ) {
            for ( int j = 0; j < width; j++ ) {
                col = callback.call(camera, sceneVoxelGrid, sceneBackground, j, i, data);
                ToneMap.radianceToRgb(col, rgb[j], toneMapOptions);
                Statistics.instance().rayTracer.pixelCount++;
            }

            SoftIds.softRenderPixels(width, 1, rgb, toneMapOptions);
        }

        finish();
    }

    /**
    Some utility routines for progressive tracing
    */
    private static void
    fillRect(
        Camera camera,
        int x0,
        int y0,
        int x1,
        int y1,
        ColorRgb col,
        ColorRgb[] rgb)
    {
        for ( int x = x0; x < x1; x++ ) {
            for ( int y = y0; y < y1; y++ ) {
                ColorRgb c = rgb[y * camera.xSize + x];
                c.set(col.getR(), col.getG(), col.getB());
            }
        }
    }

    private static void renderSegment(int width, int yMin, int yMax, ColorRgb[] rgb, ToneMappingContext toneMapOptions) {
        int height = yMax - yMin;
        if ( height <= 0 ) {
            return;
        }
        ColorRgb[] segment = new ColorRgb[width * height];
        int srcOffset = yMin * width;
        for ( int i = 0; i < segment.length; i++ ) {
            ColorRgb src = rgb[srcOffset + i];
            segment[i] = new ColorRgb(src.getR(), src.getG(), src.getB());
        }
        SoftIds.softRenderPixels(width, height, segment, toneMapOptions);
    }

    public static void
    progressive(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SCREEN_ITERATE_CALLBACK callback,
        Object data,
        ToneMappingContext toneMapOptions)
    {
        int width;
        int height;
        ColorRgb col;
        ColorRgb pixelRGB = new ColorRgb();
        ColorRgb[] rgb;
        int x0;
        int y0;
        int x1;
        int y1;
        int stepSize;
        int xSteps;
        int ySteps;
        boolean xStepDone;
        boolean yStepDone;
        boolean skip;
        int yMin;
        int yMax;

        init();

        width = camera.xSize;
        height = camera.ySize;
        rgb = new ColorRgb[width * height]; // We need a full screen!

        ColorRgb white = new ColorRgb(1.0, 1.0, 1.0);

        for ( int i = 0; i < width * height; i++ ) {
            rgb[i] = new ColorRgb(white.getR(), white.getG(), white.getB());
        }

        stepSize = 64;
        skip = false;  // First iteration all squares need to be filled
        yMin = height + 1;
        yMax = -1;

        while ( stepSize > 0 ) {
            y0 = 0;
            ySteps = 0;
            yStepDone = false;

            while ( !yStepDone ) {
                y1 = y0 + stepSize;
                if ( y1 >= height ) {
                    y1 = height;
                    yStepDone = true;
                }

                yMin = Math.min(y0, yMin);
                yMax = Math.max(y1, yMax);

                x0 = 0;
                xSteps = 0;
                xStepDone = false;

                while ( !xStepDone ) {
                    x1 = x0 + stepSize;

                    if ( x1 >= width ) {
                        x1 = width;
                        xStepDone = true;
                    }

                    if ( !skip || ((ySteps & 1) != 0) || ((xSteps & 1) != 0) ) {
                        col = callback.call(camera, sceneVoxelGrid, sceneBackground, x0, height - y0 - 1, data);
                        ToneMap.radianceToRgb(col, pixelRGB, toneMapOptions);
                        fillRect(camera, x0, y0, x1, y1, pixelRGB, rgb);

                        Statistics.instance().rayTracer.pixelCount++;

                        if ( (state.wakeUp & wakeUpRender()) != 0 ) {
                            state.wakeUp = (byte)(state.wakeUp & (~wakeUpRender()));
                            if ( (yMax > 0) && (yMax > yMin) ) {
                                renderSegment(width, yMin, yMax, rgb, toneMapOptions);
                            }
                            yMin = Math.max(0, yMax - stepSize);
                        }
                    }

                    x0 = x1;
                    xSteps++;
                }

                if ( yMax >= height ) {
                    if ( yMax > yMin ) {
                        renderSegment(width, yMin, yMax, rgb, toneMapOptions);
                    }
                    yMax = -1;
                }

                y0 = y1;
                ySteps++;
            }

            skip = true;
            stepSize /= 2;

        }

        finish();
    }
}
