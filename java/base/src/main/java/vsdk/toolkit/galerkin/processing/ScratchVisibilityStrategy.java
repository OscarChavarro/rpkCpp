/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.processing.visitors.ScratchRendererVisitor;
import vsdk.toolkit.render.sgl.SglConstants;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.environment.geometry.elements.Element;

public class ScratchVisibilityStrategy {
    /**
Create a scratch software renderer for various operations on clusters
*/
    public static void scratchInit(GalerkinState galerkinState) {
        galerkinState.scratch = new SglContext(galerkinState.scratchFrameBufferSize, galerkinState.scratchFrameBufferSize);
        galerkinState.scratch.sglDepthTesting(true);
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
    public static BoundingBox scratchRenderElements(GalerkinElement cluster, Vector3D eye, GalerkinState galerkinState) {
        BoundingBox boundingBox = new BoundingBox();

        if ( cluster.id == galerkinState.lastClusterId &&
             eye.equals(galerkinState.lastEye, Numeric.EPSILON_FLOAT) ) {
            return boundingBox;
        }

        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        galerkinState.lastClusterId = cluster.id;
        galerkinState.lastEye = eye;

        Vector3D center = cluster.midPoint();
        Vector3D up = new Vector3D(0.0, 0.0, 1.0);
        Vector3D viewDirection = new Vector3D();

        viewDirection.subtraction(center, eye);
        viewDirection.normalize(Numeric.EPSILON_FLOAT);
        if ( Math.abs(up.dotProduct(viewDirection)) > 1.0 - Numeric.EPSILON ) {
            up.set(0.0f, 1.0f, 0.0f);
        }

        Matrix4x4 lookAt = Matrix4x4.createLookAtMatrix(eye, center, up);

        Camera.transformBoundingBox(cluster.geometry.getBoundingBox(), lookAt, boundingBox);

        SglContext scratch = galerkinState.scratch;
        Matrix4x4 o = Camera.projectionMatrixFromBoundingBox(boundingBox);
        scratch.sglLoadMatrix(o);
        scratch.sglMultiplyMatrix(lookAt);

        // Choose a viewport depending on the relative size of the smallest
        // surface element in the cluster to be rendered
        int vpSize = (int)((boundingBox.dx() * boundingBox.dy()) / cluster.minimumArea);
        if ( vpSize > galerkinState.scratch.width ) {
            vpSize = galerkinState.scratch.width;
        }
        if ( vpSize < 32 ) {
            vpSize = 32;
        }
        scratch.sglViewport(0, 0, vpSize, vpSize);

        // Render element pointers in the scratch frame buffer
        scratch.sglClear(0x00, SglConstants.SGL_MAXIMUM_Z);

        ScratchRendererVisitor leafVisitor = new ScratchRendererVisitor(eye, scratch);
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);

        return boundingBox;
    }

    /**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
    public static ColorRgb scratchRadiance(GalerkinState galerkinState) {
        ColorRgb rad = new ColorRgb();
        rad.clear();
        int nonBackGround = 0;

        for ( int j = 0; j < galerkinState.scratch.vp_height; j++ ) {
            int rowStart = j * galerkinState.scratch.width;
            for ( int i = 0; i < galerkinState.scratch.vp_width; i++ ) {
                Element elementBase = galerkinState.scratch.galerkinElementBuffer[rowStart + i];
                if ( elementBase instanceof GalerkinElement ) {
                    GalerkinElement element = (GalerkinElement)elementBase;
                    if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
                         galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
                        rad.add(rad, element.radiance[0]);
                    }
                    else {
                        rad.add(rad, element.unShotRadiance[0]);
                    }
                    nonBackGround++;
                }
            }
        }
        if ( nonBackGround > 0 ) {
            rad.scale(1.0f / (float)(galerkinState.scratch.vp_width * galerkinState.scratch.vp_height));
        }
        return rad;
    }

    /**
Computes the number of non background pixels
*/
    public static int scratchNonBackgroundPixels(GalerkinState galerkinState) {
        int nonBackGround = 0;

        for ( int j = 0; j < galerkinState.scratch.vp_height; j++ ) {
            int rowStart = j * galerkinState.scratch.width;
            for ( int i = 0; i < galerkinState.scratch.vp_width; i++ ) {
                Element elementBase = galerkinState.scratch.galerkinElementBuffer[rowStart + i];
                if ( elementBase != null ) {
                    nonBackGround++;
                }
            }
        }
        return nonBackGround;
    }

    /**
Counts the number of pixels occupied by each element. The result is
accumulated in the tmp field of the elements. This field should be
initialized to zero before
*/
    public static void scratchPixelsPerElement(GalerkinState galerkinState) {
        for ( int i = 0; i < galerkinState.scratch.vp_height; i++ ) {
            int rowStart = i * galerkinState.scratch.width;
            for ( int j = 0; j < galerkinState.scratch.vp_width; j++ ) {
                Element elementBase = galerkinState.scratch.galerkinElementBuffer[rowStart + j];
                if ( elementBase instanceof GalerkinElement ) {
                    GalerkinElement elem = (GalerkinElement)elementBase;
                    elem.scratchVisibilityUsageCounter++;
                }
            }
        }
    }
}
