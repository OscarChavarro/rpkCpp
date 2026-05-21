package vsdk.toolkit.material;

import vsdk.toolkit.common.color.ColorRgb;

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTON MAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class

Build-time feature flags are configured from CMake.
*/

// Color in which to draw outlines

// Color in which to draw bounding boxes

// Color in which to show cluster bounding boxes

// True for drawing facet outlines

// False for using any kind of shading

// True for rendering with Gouraud interpolation

// True for backface culling

// True for showing bounding boxes

// True for showing cluster hierarchy

// Frustum culling accelerates rendering of large scenes.

// For freezing ray-traced image on the screen when appropriate

// High-dynamic range ray-traced tiff

public class RenderOptions {
    public ColorRgb outlineColor;
    public ColorRgb boundingBoxColor;
    public ColorRgb clusterColor;
    public float lineWidth;
    public boolean drawOutlines;
    public boolean drawSurfaces;
    public boolean noShading;
    public boolean smoothShading;
    public boolean backfaceCulling;
    public boolean drawBoundingBoxes;
    public boolean drawClusters;
    public boolean frustumCulling;
    public boolean renderRayTracedImage;
    public boolean trace;

    private static final boolean DEFAULT_SMOOTH_SHADING = true;
    private static final boolean DEFAULT_BACKFACE_CULLING = true;
    private static final boolean DEFAULT_OUTLINE_DRAWING = false;
    private static final boolean DEFAULT_SURFACE_DRAWING = true;
    private static final boolean DEFAULT_BOUNDING_BOX_DRAWING = false;
    private static final boolean DEFAULT_CLUSTER_DRAWING = false;
    private static final ColorRgb DEFAULT_OUTLINE_COLOR = new ColorRgb(0.5f, 0.0f, 0.0f);
    private static final ColorRgb DEFAULT_BOUNDING_BOX_COLOR = new ColorRgb(0.5f, 0.0f, 1.0f);
    private static final ColorRgb DEFAULT_CLUSTER_COLOR = new ColorRgb(1.0f, 0.5f, 0.0f);

    public RenderOptions() {
        outlineColor = new ColorRgb();
        boundingBoxColor = new ColorRgb();
        clusterColor = new ColorRgb();

        smoothShading = DEFAULT_SMOOTH_SHADING;
        backfaceCulling = DEFAULT_BACKFACE_CULLING;
        drawSurfaces = DEFAULT_SURFACE_DRAWING;
        drawOutlines = DEFAULT_OUTLINE_DRAWING;
        drawBoundingBoxes = DEFAULT_BOUNDING_BOX_DRAWING;
        drawClusters = DEFAULT_CLUSTER_DRAWING;

        outlineColor = new ColorRgb(DEFAULT_OUTLINE_COLOR.getR(), DEFAULT_OUTLINE_COLOR.getG(), DEFAULT_OUTLINE_COLOR.getB());
        boundingBoxColor = new ColorRgb(DEFAULT_BOUNDING_BOX_COLOR.getR(), DEFAULT_BOUNDING_BOX_COLOR.getG(), DEFAULT_BOUNDING_BOX_COLOR.getB());
        clusterColor = new ColorRgb(DEFAULT_CLUSTER_COLOR.getR(), DEFAULT_CLUSTER_COLOR.getG(), DEFAULT_CLUSTER_COLOR.getB());

        frustumCulling = false;
        noShading = false;
        lineWidth = 1.0f;
        renderRayTracedImage = false;
        trace = false;
    }
}
