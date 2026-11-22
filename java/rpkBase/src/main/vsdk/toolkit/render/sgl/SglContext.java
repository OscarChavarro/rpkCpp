/**
Small Graphics Library
*/

package vsdk.toolkit.render.sgl;

/**
Small Graphics Library. Software rendering into a user
accessible memory buffer. E.getG(). for clustering where a small number
of patches needs to be ID rendered very often
*/

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4f;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.common.linealAlgebra.Vector4Df;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class SglContext {
    private static final Matrix4x4f IDENTITY_MATRIX = new Matrix4x4f();

    /** The current transform is the entry 0 of the transform stack */
    private void setCurrentTransform(Matrix4x4f transform) {
        transformStack[0] = transform;
        currentTransform = transform;
    }

    private static void clearFrameBuffer(SglContext sglContext, long backgroundColor) {
        int viewportOrigin = sglContext.vp_y * sglContext.width + sglContext.vp_x;
        for (int j = 0; j < sglContext.vp_height; j++) {
            int rowStart = viewportOrigin + j * sglContext.width;
            for (int i = 0; i < sglContext.vp_width; i++) {
                int pixelIndex = rowStart + i;
                sglContext.frameBuffer[pixelIndex] = backgroundColor;
                sglContext.patchBuffer[pixelIndex] = null;
                sglContext.galerkinElementBuffer[pixelIndex] = null;
            }
        }
    }

    private static Matrix4x4f identityMatrix() {
        return IDENTITY_MATRIX;
    }

    public Matrix4x4f[] transformStack; // Transform stack
    public Matrix4x4f currentTransform;
    public boolean clipping; // Whether to do clipping or not
    public int vp_x; // Viewport
    public int vp_y;
    public double near; // Depth range
    public double far;

    public SglPixelContent pixelData;
    public long[] frameBuffer;
    public Patch[] patchBuffer;
    public Element[] galerkinElementBuffer;

    public long currentPixel;
    public Patch currentPatch;
    public Element currentGalerkinElement;

    public long[] depthBuffer; // Z buffer

    public int width; // canvas size
    public int height;
    public int vp_width;
    public int vp_height;

    /**
Creates and destroys an SGL rendering context.
*/
    public SglContext(int width, int height) {
        transformStack = new Matrix4x4f[SglConstants.SGL_TRANSFORM_STACK_SIZE];
        for (int i = 0; i < transformStack.length; i++) {
            transformStack[i] = new Matrix4x4f();
        }

        // Frame buffer
        this.width = width;
        this.height = height;
        frameBuffer = new long[width * height];
        patchBuffer = new Patch[width * height];
        galerkinElementBuffer = new Element[width * height];

        for (int i = 0; i < width * height; i++) {
            frameBuffer[i] = 0;
            patchBuffer[i] = null;
            galerkinElementBuffer[i] = null;
        }

        pixelData = SglPixelContent.PIXEL;

        // No Z buffer
        depthBuffer = null;

        // Transform stack and current transform
        setCurrentTransform(identityMatrix());

        currentPixel = 0;
        currentPatch = null;
        currentGalerkinElement = null;

        clipping = true;

        // Default viewport and depth range
        vp_x = 0;
        vp_y = 0;
        vp_width = width;
        vp_height = height;
        near = 0.0;
        far = 1.0;
    }

    /**
Returns current sgl renderer
*/
    public void sglClearZBuffer(long defZVal) {
        int viewportOrigin = vp_y * width + vp_x;
        for (int j = 0; j < vp_height; j++) {
            int rowStart = viewportOrigin + j * width;
            for (int i = 0; i < vp_width; i++) {
                depthBuffer[rowStart + i] = defZVal;
            }
        }
    }

    public void sglClear(long backgroundColor, long defZVal) {
        SglContext.clearFrameBuffer(this, backgroundColor);
        sglClearZBuffer(defZVal);
    }

    public void sglDepthTesting(boolean on) {
        if (on) {
            if (depthBuffer != null) {
                return;
            }
            else {
                depthBuffer = new long[width * height];
            }
        }
        else {
            if (depthBuffer != null) {
                depthBuffer = null;
            }
            else {
                return;
            }
        }
    }

    public void sglClipping(boolean on) {
        clipping = on;
    }

    public void sglLoadMatrix(Matrix4x4f xf) {
        setCurrentTransform(xf);
    }

    public void sglMultiplyMatrix(Matrix4x4f xf) {
        setCurrentTransform(currentTransform.multiply(xf));
    }

    public void sglSetPatch(Patch patch) {
        pixelData = SglPixelContent.PATCH_POINTER;
        currentPatch = patch;
    }

    public void sglSetGalerkinElement(Element galerkinElement) {
        pixelData = SglPixelContent.ELEMENT_POINTER;
        currentGalerkinElement = galerkinElement;
    }

    public void sglViewport(int x, int y, int viewPortWidth, int viewPortHeight) {
        vp_x = x;
        vp_y = y;
        vp_width = viewPortWidth;
        vp_height = viewPortHeight;
    }

    public void sglPolygon(int numberOfVertices, Vector3Df[] vertices) {
        Polygon pol = new Polygon();
        Window win = new Window();
        PolygonBox clip_box = new PolygonBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

        if (numberOfVertices > (clipping ? (PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON - 6) : PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON)) {
            Logger.reportMessage(this, Logger.ERROR, "sglPolygon", String.format("Too many vertices (max. %d)", PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON));
            return;
        }

        // Transform the vertices and fill in a Poly
        for (int i = 0; i < numberOfVertices; i++) {
            PolygonVertex vertex = pol.vertices[i];
            final Vector4Df v = currentTransform.multiply(
                new Vector4Df(vertices[i].x(), vertices[i].y(), vertices[i].z(), 1.0f));
            if (v.w() > -VSDK.EPSILON && v.w() < VSDK.EPSILON) {
                return;
            }
            vertex.sx = v.x();
            vertex.sy = v.y();
            vertex.sz = v.z();
            vertex.sw = v.w();
        }
        pol.n = numberOfVertices;
        pol.mask = 0;

        if (clipping) {
            pol.mask = Poly.mask(0L) |
                Poly.mask(Double.BYTES) |
                Poly.mask(2L * Double.BYTES) |
                Poly.mask(3L * Double.BYTES);
            if (Poly.clipToBox(pol, clip_box) == PolygonClipResult.POLY_CLIP_OUT) {
                return;
            }
        }

        // Perspective divide and transformation to viewport and depth range
        for (int i = 0; i < pol.n; i++) {
            PolygonVertex vertex = pol.vertices[i];
            vertex.sx = (double)vp_x + (vertex.sx / vertex.sw + 1.0) * (double)vp_width * 0.5;
            vertex.sy = (double)vp_y + (vertex.sy / vertex.sw + 1.0) * (double)vp_height * 0.5;
            vertex.sz = (near + (vertex.sz / vertex.sw + 1.0) * far * 0.5) * (double)SglConstants.SGL_MAXIMUM_Z;
        }

        // Window
        win.x0 = vp_x;
        win.y0 = vp_y;
        win.x1 = vp_x + vp_width - 1;
        win.y1 = vp_y + vp_height - 1;

        // Scan convert the polygon: use optimized version for flat shading with or without Z buffering
        if (depthBuffer != null) {
            Poly.scanZ(this, pol, win);
        }
        else {
            Poly.scanFlat(this, pol, win);
        }
    }
}
