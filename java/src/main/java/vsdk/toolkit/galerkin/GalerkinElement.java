/**
Galerkin finite elements: one structure for both surface and cluster elements
*/

package vsdk.toolkit.galerkin;

import java.util.ArrayList;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.ElementFlags;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;

/**
The Galerkin radiosity specific data to be kept with every surface or
cluster element. A flag indicates whether a given element is a cluster or
surface elements. There are only a few differences between surface and cluster
elements: cluster elements always have a constant basis on them, they contain
a pointer to the Geometry to which they are associated, while a surface element has
a pointer to the Patch to which is belongs, they have only irregular sub-elements,
and no up trans
*/
public class GalerkinElement extends Element {
    private static int numberOfElements = 0;
    private static int numberOfClusters = 0;

    public Patch patch;
    public Geometry geometry;
    public float potential; // Total potential of the element
    public float receivedPotential; // Potential received during the last iteration
    public float unShotPotential; // Un-shot potential (progressive refinement radiosity)
    public float directPotential;
    public ArrayList<Object> interactions; // Links with other patches: when using
                                           // a shooting algorithm, the links are kept with the source element. When doing gathering,
                                           // the links are kept with the receiver element
    public float minimumArea; // Equal to area for a surface element or the area
                              // of the smallest surface element in a cluster
    public float blockerSize; // Equivalent blocker size for multi-resolution visibility
    public int numberOfPatches; // Number of patches in a cluster
    public int scratchVisibilityUsageCounter; // Used only on Z-depth visibility clustering strategy
    public int childNumber; // Rang nr of regular sub-element in parent
    public byte basisSize; // Number of coefficients to represent radiance
    public byte basisUsed; // Number of coefficients effectively used (<=basis_size)
    public GalerkinState galerkinState;

    private GalerkinElement(GalerkinState inGalerkinState) {
        super();
        numberOfElements++;
        id = numberOfElements;
        patch = null;
        geometry = null;
        potential = 0.0f;
        receivedPotential = 0.0f;
        unShotPotential = 0.0f;
        directPotential = 0.0f;
        interactions = null;
        minimumArea = 0.0f;
        blockerSize = 0.0f;
        numberOfPatches = 0;
        scratchVisibilityUsageCounter = 0;
        childNumber = 0;
        basisSize = 1;
        basisUsed = 1;
        galerkinState = inGalerkinState;

        reAllocCoefficients();
    }

    public GalerkinElement(Patch inPatch, GalerkinState inGalerkinState) {
        this(inGalerkinState);
        patch = inPatch;
        geometry = null;
        flags &= ~ElementFlags.IS_CLUSTER_MASK;
        area = inPatch == null ? 0.0f : inPatch.area;
        minimumArea = area;
        numberOfPatches = inPatch == null ? 0 : 1;
    }

    public GalerkinElement(Geometry inGeometry, GalerkinState inGalerkinState) {
        this(inGalerkinState);
        geometry = inGeometry;
        patch = null;
        flags |= ElementFlags.IS_CLUSTER_MASK;
        numberOfClusters++;
    }

    public static int getNumberOfElements() {
        return numberOfElements;
    }

    public static int getNumberOfClusters() {
        return numberOfClusters;
    }

    public static int getNumberOfSurfaceElements() {
        return numberOfElements - numberOfClusters;
    }

    public static GalerkinElement fromPatch(Patch patch) {
        if ( patch == null || patch.radianceData == null || !(patch.radianceData instanceof GalerkinElement) ) {
            return null;
        }
        return (GalerkinElement)patch.radianceData;
    }

    public static void initializeBasis() {
    }

    public static int renderMode(RenderOptions renderOptions) {
        if ( renderOptions == null ) {
            return 0;
        }
        return renderOptions.smoothShading ? 1 : 0;
    }

    public void regularSubDivide() {
    }

    public GalerkinElement regularLeafAtPoint(double[] u, double[] v) {
        if ( u != null && v != null ) {
            // Parameters intentionally unused in this reduced Java migration.
        }
        return this;
    }

    public int vertices(Vector3D[] p) {
        if ( patch == null || p == null ) {
            return 0;
        }
        int count = Math.min(patch.numberOfVertices, p.length);
        for ( int i = 0; i < count; i++ ) {
            if ( patch.vertex[i] != null && patch.vertex[i].point != null ) {
                p[i].set(patch.vertex[i].point.x, patch.vertex[i].point.y, patch.vertex[i].point.z);
            }
        }
        return count;
    }

    public BoundingBox bounds(BoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return null;
        }

        if ( patch != null ) {
            patch.computeAndGetBoundingBox(boundingBox);
            return boundingBox;
        }
        if ( geometry != null ) {
            boundingBox.copyFrom(geometry.boundingBox);
            return boundingBox;
        }
        return boundingBox;
    }

    public Vector3D midPoint() {
        Vector3D p = new Vector3D();
        if ( patch != null && patch.midPoint != null ) {
            p.copy(patch.midPoint);
        }
        else if ( geometry != null ) {
            p = geometry.boundingBox.center();
        }
        return p;
    }

    public void initPolygon(Polygon polygon) {
        if ( polygon == null || patch == null ) {
            return;
        }

        polygon.normal.set(patch.normal.x, patch.normal.y, patch.normal.z);
        polygon.planeConstant = patch.planeConstant;
        polygon.numberOfVertices = patch.numberOfVertices;
        polygon.bounds.copyFrom(patch.boundingBox);
        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            if ( patch.vertex[i] != null && patch.vertex[i].point != null ) {
                polygon.vertex[i].set(
                    patch.vertex[i].point.x,
                    patch.vertex[i].point.y,
                    patch.vertex[i].point.z);
            }
        }
    }

    public void reAllocCoefficients() {
        int size = basisSize <= 0 ? 1 : basisSize;

        radiance = new ColorRgb[size];
        receivedRadiance = new ColorRgb[size];
        unShotRadiance = new ColorRgb[size];

        for ( int i = 0; i < size; i++ ) {
            radiance[i] = new ColorRgb();
            receivedRadiance[i] = new ColorRgb();
            unShotRadiance[i] = new ColorRgb();
        }
    }

    public Patch getPatch() {
        return patch;
    }

    public void setPatch(Patch inPatch) {
        patch = inPatch;
    }
}
