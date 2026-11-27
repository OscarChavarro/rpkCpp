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
import vsdk.toolkit.galerkin.GalerkinElementVisibilityBuffer;
import vsdk.toolkit.render.sgl.Window;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

public class ScratchVisibilityStrategy {
    /**
Create a scratch software renderer for various operations on clusters
*/
    public static void scratchInit(GalerkinState galerkinState) {
        galerkinState.scratch = new GalerkinElementVisibilityBuffer(
            galerkinState.scratchFrameBufferSize, galerkinState.scratchFrameBufferSize, null);
    }

    /**
Terminates scratch rendering
*/
    public static void scratchTerminate(GalerkinState galerkinState) {
        if ( galerkinState.scratch != null ) {
            galerkinState.scratch = null;
        }
    }

    /**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer and returns pointer to a bounding box
containing the size of the virtual screen. The cluster nicely fits
into the virtual screen
*/
    public static RpkAxisAlignedBoundingBox scratchRenderElements(GalerkinElement cluster, Vector3Dd eye, GalerkinState galerkinState) {
        RpkAxisAlignedBoundingBox boundingBox = galerkinState.getScratchBounds();

        if ( cluster.id == galerkinState.lastClusterId &&
             Numeric.vectorsEqual(eye, galerkinState.lastEye, Numeric.EPSILON) ) {
            return boundingBox;
        }

        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        galerkinState.lastClusterId = cluster.id;
        galerkinState.lastEye = eye;

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

        GalerkinElementVisibilityBuffer scratch = galerkinState.scratch;
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
    public static ColorRgb scratchRadiance(GalerkinState galerkinState) {
        double[] rad = {0.0, 0.0, 0.0};

        final GalerkinElementVisibilityBuffer scratch = galerkinState.scratch;
        final Window viewport = scratch.getViewport();

        int nonBackground = 0;
        for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
            for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
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
    public static int scratchNonBackgroundPixels(GalerkinState galerkinState) {
        final GalerkinElementVisibilityBuffer scratch = galerkinState.scratch;
        final Window viewport = scratch.getViewport();
        int nonBackground = 0;

        for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
            for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
                if ( scratch.getSample(x, y) != null ) {
                    nonBackground++;
                }
            }
        }
        return nonBackground;
    }

    /** Accumulates coverage on the solver's top-level surface elements. */
    public static void scratchPixelsPerElement(GalerkinState galerkinState) {
        final GalerkinElementVisibilityBuffer scratch = galerkinState.scratch;
        final Window viewport = scratch.getViewport();

        for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
            for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
                final GalerkinElement element = scratch.getSample(x, y);
                if ( element != null ) {
                    // Cluster leaves are the same top-level elements bound to the scene faces.
                    galerkinState.topLevelElement(element.patch).scratchVisibilityUsageCounter++;
                }
            }
        }
    }
}
