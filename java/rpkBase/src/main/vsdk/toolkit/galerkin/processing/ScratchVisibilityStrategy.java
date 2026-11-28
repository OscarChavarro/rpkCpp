/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.processing.visitors.ScratchRendererVisitor;
import vsdk.toolkit.render.raster.polygonScanConverter.PolygonVisibilityBuffer;
import vsdk.toolkit.render.raster.polygonScanConverter.RasterRectangle2D;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

/**
Every operation works on the scratch workspace it is given, the solver's or a
parallel Jacobi worker's, and reads the solver state only
*/
public class ScratchVisibilityStrategy {
    /**
Create a scratch software renderer for various operations on clusters
*/
    public static void scratchInit(GalerkinState galerkinState) {
        galerkinState.scratchVisibilityWorkspace = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
    }

    /**
Terminates scratch rendering
*/
    public static void scratchTerminate(GalerkinState galerkinState) {
        if ( galerkinState.scratchVisibilityWorkspace != null ) {
            galerkinState.scratchVisibilityWorkspace = null;
        }
    }

    /**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer of the workspace and returns the bounding
box containing the size of the virtual screen. The cluster nicely fits
into the virtual screen.
Java adaptation: the C++ port returns a copy of the bounding box; here it is
the workspace's own, valid until the workspace renders again
*/
    public static RpkAxisAlignedBoundingBox scratchRenderElements(
        GalerkinElement cluster,
        Vector3Dd eye,
        ScratchVisibilityWorkspace scratchWorkspace,
        GalerkinState galerkinState)
    {
        RpkAxisAlignedBoundingBox boundingBox = scratchWorkspace.getBounds();

        if ( scratchWorkspace.holdsRendering(cluster, eye) ) {
            return boundingBox;
        }

        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        scratchWorkspace.setRendering(cluster, eye);

        final Vector3Dd center = cluster.midPoint();
        Vector3Dd up = new Vector3Dd(0.0, 0.0, 1.0);
        Vector3Dd viewDirection;

        viewDirection = center.subtract(eye);
        viewDirection = viewDirection.normalized(Numeric.EPSILON);
        if ( Math.abs(up.dotProduct(viewDirection)) > 1.0 - VSDK.EPSILON ) {
            up = new Vector3Dd(0.0, 1.0, 0.0);
        }

        final Matrix4x4d lookAt = Camera.lookAtMatrix(eye, center, up);

        Camera.transformBoundingBox(cluster.geometry.getBoundingBox(), lookAt, boundingBox);

        PolygonVisibilityBuffer<GalerkinElement> scratch = scratchWorkspace.getBuffer();
        final Matrix4x4d projection = Camera.projectionMatrixFromBoundingBox(boundingBox);
        scratch.setTransform(projection.multiply(lookAt));

        // Choose a viewport based on the smallest surface element in the cluster.
        int viewportSize = (int)((boundingBox.dx() * boundingBox.dy()) / cluster.minimumArea);
        if ( viewportSize < 32 ) {
            viewportSize = 32;
        }
        if ( viewportSize > scratch.getWidth() ) {
            viewportSize = scratch.getWidth();
        }
        scratch.setViewport(0, 0, viewportSize, viewportSize);
        scratch.clear();

        ScratchRendererVisitor leafVisitor = new ScratchRendererVisitor(eye, scratch);
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);

        return boundingBox;
    }

    /**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
    public static ColorRgb scratchRadiance(ScratchVisibilityWorkspace scratchWorkspace, GalerkinState galerkinState) {
        double[] rad = {0.0, 0.0, 0.0};

        final PolygonVisibilityBuffer<GalerkinElement> scratch = scratchWorkspace.getBuffer();
        final RasterRectangle2D viewport = scratch.getViewport();

        int nonBackground = 0;
        for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
            for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
                final GalerkinElement element = scratch.getSample(x, y);
                if ( element != null ) {
                    if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
                         galerkinState.isJacobi() ) {
                        rad[0] += element.radiance[0].getR();
                        rad[1] += element.radiance[0].getG();
                        rad[2] += element.radiance[0].getB();
                    } else {
                        rad[0] += element.unShotRadiance[0].getR();
                        rad[1] += element.unShotRadiance[0].getG();
                        rad[2] += element.unShotRadiance[0].getB();
                    }
                    nonBackground++;
                }
            }
        }
        if ( nonBackground > 0 ) {
            final double scale = 1.0 / (double)(scratch.getViewportWidth() * scratch.getViewportHeight());
            rad[0] *= scale;
            rad[1] *= scale;
            rad[2] *= scale;
        }
        return new ColorRgb(rad[0], rad[1], rad[2]);
    }

    /** Computes the number of covered samples in the scratch viewport. */
    public static int scratchNonBackgroundPixels(ScratchVisibilityWorkspace scratchWorkspace) {
        final PolygonVisibilityBuffer<GalerkinElement> scratch = scratchWorkspace.getBuffer();
        final RasterRectangle2D viewport = scratch.getViewport();
        int nonBackground = 0;

        for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
            for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
                if ( scratch.getSample(x, y) != null ) {
                    nonBackground++;
                }
            }
        }
        return nonBackground;
    }

    /**
Counts, in the workspace, the samples covered by each of the solver's
top-level surface elements in the last rendering
*/
    public static void scratchPixelsPerElement(ScratchVisibilityWorkspace scratchWorkspace, GalerkinState galerkinState) {
        final PolygonVisibilityBuffer<GalerkinElement> scratch = scratchWorkspace.getBuffer();
        final RasterRectangle2D viewport = scratch.getViewport();

        scratchWorkspace.startCounting();
        // Neighbouring samples mostly show the same element: count runs of equal
        // samples and add each run to its element at once
        GalerkinElement runElement = null;
        int runLength = 0;
        for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
            for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
                final GalerkinElement element = scratch.getSample(x, y);
                if ( element == runElement ) {
                    runLength++;
                } else {
                    addSamples(scratchWorkspace, runElement, runLength, galerkinState);
                    runElement = element;
                    runLength = 1;
                }
            }
        }
        addSamples(scratchWorkspace, runElement, runLength, galerkinState);
    }

    /** Adds the samples of a run showing the element (none for the background) */
    private static void addSamples(
        ScratchVisibilityWorkspace scratchWorkspace,
        GalerkinElement element,
        int samples,
        GalerkinState galerkinState)
    {
        if ( element == null || samples == 0 ) {
            return;
        }
        // Cluster leaves are the same top-level elements bound to the scene faces.
        GalerkinElement topLevel = galerkinState.topLevelElement(element.patch);
        scratchWorkspace.addSamples(topLevel, topLevel.id, samples);
    }

    /** Area of the virtual screen that one sample of the last rendering covers */
    public static double scratchPixelArea(RpkAxisAlignedBoundingBox boundingBox, ScratchVisibilityWorkspace scratchWorkspace) {
        final PolygonVisibilityBuffer<GalerkinElement> scratch = scratchWorkspace.getBuffer();
        return boundingBox.dx() * boundingBox.dy() /
            (double)(scratch.getViewportWidth() * scratch.getViewportHeight());
    }
}
