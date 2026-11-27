/**
Galerkin finite elements: one structure for both surface and cluster elements
*/

package vsdk.toolkit.galerkin;

import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.QuadCubatureRule;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.numericalAnalysis.TriangleCubatureRule;
import vsdk.toolkit.scene.ConvexPolygon;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

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
    public RpkGeometry geometry;
    public double potential; // Total potential of the element
    public double receivedPotential; // Potential received during the last iteration
    public double unShotPotential; // Un-shot potential (progressive refinement radiosity)
    public double directPotential;
    public ArrayList<Object> interactions; // Links with other patches: when using
                                           // a shooting algorithm, the links are kept with the source element. When doing gathering,
                                           // the links are kept with the receiver element
    public double minimumArea; // Equal to area for a surface element or the area
                              // of the smallest surface element in a cluster
    public double blockerSize; // Equivalent blocker size for multi-resolution visibility
    public int numberOfPatches; // Number of patches in a cluster
    public int scratchVisibilityUsageCounter; // Used only on Z-depth visibility clustering strategy
    public int childNumber; // Rang nr of regular sub-element in parent
    public byte basisSize; // Number of coefficients to represent radiance
    public byte basisUsed; // Number of coefficients effectively used (<=basis_size)
    public GalerkinState galerkinState;

    private static final Matrix2x2[] quadToParentTransformMatrix = buildQuadTransforms();
    private static final Matrix2x2[] triangleToParentTransformMatrix = buildTriangleTransforms();

    private static Matrix2x2[] buildQuadTransforms() {
        return new Matrix2x2[] {
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.0),
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.0),
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.5),
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.5)
        };
    }

    private static Matrix2x2[] buildTriangleTransforms() {
        return new Matrix2x2[] {
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.0),
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.0),
            new Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.5),
            new Matrix2x2(-0.5, 0.0, 0.0, -0.5, 0.5, 0.5)
        };
    }

    private GalerkinElement(GalerkinState inGalerkinState) {
        super();
        numberOfElements++;
        id = numberOfElements;
        className = ElementTypes.ELEMENT_GALERKIN;
        patch = null;
        geometry = null;
        potential = 0.0;
        receivedPotential = 0.0;
        unShotPotential = 0.0;
        directPotential = 0.0;
        interactions = new ArrayList<>();
        minimumArea = 0.0;
        blockerSize = 0.0;
        numberOfPatches = 0;
        scratchVisibilityUsageCounter = 0;
        childNumber = 0;
        basisSize = 0;
        basisUsed = 0;
        galerkinState = inGalerkinState;
    }

    public GalerkinElement(Patch inPatch, GalerkinState inGalerkinState) {
        this(inGalerkinState);
        patch = inPatch;
        geometry = null;
        flags &= ~ElementFlags.IS_CLUSTER_MASK;
        area = inPatch == null ? 0.0 : inPatch.getArea();
        minimumArea = area;
        blockerSize = 2.0 * (double)Math.sqrt(area / Math.PI);
        numberOfPatches = inPatch == null ? 0 : 1;
        directPotential = 0.0; // Every face starts without direct potential (DirectPotentialTable)

        if ( inPatch != null ) {
            Rd = PatchVisitor.averageNormalAlbedo(inPatch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
            if ( inPatch.getMaterial() != null && inPatch.getMaterial().getEdf() != null ) {
                flags |= ElementFlags.IS_LIGHT_SOURCE_MASK;
                Ed = PatchVisitor.averageEmittance(inPatch, XxdfComponentFlag.DIFFUSE_COMPONENT);
                Ed = Ed.scaleInverse(Math.PI);
            }
        }

        reAllocCoefficients();
    }

    public GalerkinElement(RpkGeometry inGeometry, GalerkinState inGalerkinState) {
        this(inGalerkinState);
        geometry = inGeometry;
        patch = null;
        flags |= ElementFlags.IS_CLUSTER_MASK;
        Rd = new ColorRgb(1.0, 1.0, 1.0);
        reAllocCoefficients();
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

    /**
    Checked lookup of the top-level element of a face; null when the face has
    no Galerkin element
    */
    public static GalerkinElement fromPatch(GalerkinState galerkinState, Patch patch) {
        Element element = galerkinState.boundElement(patch);
        if ( !(element instanceof GalerkinElement) ) {
            return null;
        }
        return (GalerkinElement)element;
    }

    public static void initializeBasis() {
        GalerkinBasis.computeRegularFilterCoefficients(
            GalerkinBasis.mutableBasisForVertexCount(4),
            quadToParentTransformMatrix,
            QuadCubatureRule.degree8QuadrilateralRule());
        GalerkinBasis.computeRegularFilterCoefficients(
            GalerkinBasis.mutableBasisForVertexCount(3),
            triangleToParentTransformMatrix,
            TriangleCubatureRule.degree8Rule());
    }

    public static int renderMode(RendererConfiguration renderOptions) {
        if ( renderOptions == null ) {
            return GalerkinElementRenderMode.FLAT.value;
        }

        int renderCode = 0;
        if ( renderOptions.drawOutlines ) {
            renderCode |= GalerkinElementRenderMode.OUTLINE.value;
        }
        if ( renderOptions.smoothShading ) {
            renderCode |= GalerkinElementRenderMode.GOURAUD.value;
        }
        else {
            renderCode |= GalerkinElementRenderMode.FLAT.value;
        }

        return renderCode;
    }

    public void regularSubDivide() {
        if ( isCluster() ) {
            return;
        }
        if ( regularSubElements != null || patch == null ) {
            return;
        }

        Element[] children = new Element[4];
        for ( int i = 0; i < 4; i++ ) {
            GalerkinElement child = new GalerkinElement(galerkinState);
            child.patch = patch;
            child.parent = this;
            child.transformToParent = (patch.getNumberOfVertices() == 3)
                ? triangleToParentTransformMatrix[i]
                : quadToParentTransformMatrix[i];
            child.area = 0.25 * area;
            child.blockerSize = 2.0 * (double)Math.sqrt(child.area / Math.PI);
            child.childNumber = i;
            child.reAllocCoefficients();
            GalerkinBasis.push(this, radiance, child, child.radiance);
            child.potential = potential;
            child.directPotential = directPotential;
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                GalerkinBasis.push(this, unShotRadiance, child, child.unShotRadiance);
                child.unShotPotential = unShotPotential;
            }
            child.flags |= (flags & ElementFlags.IS_LIGHT_SOURCE_MASK);
            child.Rd = Rd;
            child.Ed = Ed;
            children[i] = child;
        }
        regularSubElements = children;
    }

    private GalerkinElement regularSubElementAtPoint(double[] u, double[] v) {
        if ( isCluster() || regularSubElements == null || patch == null || u == null || v == null ) {
            return this;
        }

        Element childElement;
        double uu = u[0];
        double vv = v[0];
        switch ( patch.getNumberOfVertices() ) {
            case 3:
                if ( uu + vv <= 0.5 ) {
                    childElement = regularSubElements[0];
                    u[0] = uu * 2.0;
                    v[0] = vv * 2.0;
                } else if ( uu > 0.5 ) {
                    childElement = regularSubElements[1];
                    u[0] = (uu - 0.5) * 2.0;
                    v[0] = vv * 2.0;
                } else if ( vv > 0.5 ) {
                    childElement = regularSubElements[2];
                    u[0] = uu * 2.0;
                    v[0] = (vv - 0.5) * 2.0;
                } else {
                    childElement = regularSubElements[3];
                    u[0] = (0.5 - uu) * 2.0;
                    v[0] = (0.5 - vv) * 2.0;
                }
                break;
            case 4:
                if ( vv <= 0.5 ) {
                    if ( uu < 0.5 ) {
                        childElement = regularSubElements[0];
                        u[0] = uu * 2.0;
                    } else {
                        childElement = regularSubElements[1];
                        u[0] = (uu - 0.5) * 2.0;
                    }
                    v[0] = vv * 2.0;
                } else {
                    if ( uu < 0.5 ) {
                        childElement = regularSubElements[2];
                        u[0] = uu * 2.0;
                    } else {
                        childElement = regularSubElements[3];
                        u[0] = (uu - 0.5) * 2.0;
                    }
                    v[0] = (vv - 0.5) * 2.0;
                }
                break;
            default:
                return this;
        }

        if ( childElement instanceof GalerkinElement ) {
            return (GalerkinElement)childElement;
        }
        return this;
    }

    public GalerkinElement regularLeafAtPoint(double[] u, double[] v) {
        GalerkinElement leaf = this;
        while ( leaf.regularSubElements != null ) {
            leaf = leaf.regularSubElementAtPoint(u, v);
        }
        return leaf;
    }

    public int vertices(Vector3Dd[] p) {
        if ( p == null ) {
            return 0;
        }

        if ( isCluster() ) {
            RpkAxisAlignedBoundingBox boundingBox = new RpkAxisAlignedBoundingBox();
            bounds(boundingBox);
            boundingBox.corners(p);
            return 8;
        }

        if ( patch == null ) {
            return 0;
        }

        Matrix2x2 topTrans = null;
        Vector2Dd uv;
        if ( transformToParent != null ) {
            topTrans = topTransform();
        }

        uv = new Vector2Dd(0.0, 0.0);
        if ( transformToParent != null ) uv = topTrans.transformPoint(uv);
        p[0] = patch.uniformPoint(uv.x(), uv.y());

        uv = new Vector2Dd(1.0, 0.0);
        if ( transformToParent != null ) uv = topTrans.transformPoint(uv);
        p[1] = patch.uniformPoint(uv.x(), uv.y());

        if ( patch.getNumberOfVertices() == 4 ) {
            uv = new Vector2Dd(1.0, 1.0);
            if ( transformToParent != null ) uv = topTrans.transformPoint(uv);
            p[2] = patch.uniformPoint(uv.x(), uv.y());

            uv = new Vector2Dd(0.0, 1.0);
            if ( transformToParent != null ) uv = topTrans.transformPoint(uv);
            p[3] = patch.uniformPoint(uv.x(), uv.y());
        }
        else {
            uv = new Vector2Dd(0.0, 1.0);
            if ( transformToParent != null ) uv = topTrans.transformPoint(uv);
            p[2] = patch.uniformPoint(uv.x(), uv.y());
            if ( p.length > 3 ) {
                p[3] = new Vector3Dd(0.0, 0.0, 0.0);
            }
        }

        return patch.getNumberOfVertices();
    }

    public RpkAxisAlignedBoundingBox bounds(RpkAxisAlignedBoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return null;
        }

        if ( isCluster() && geometry != null ) {
            boundingBox.copyFrom(geometry.boundingBox);
            return boundingBox;
        }

        Vector3Dd[] p = new Vector3Dd[] {new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()};
        int numberOfVertices = vertices(p);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            boundingBox.enlargeToIncludePoint(p[i]);
        }
        return boundingBox;
    }

    public Vector3Dd midPoint() {
        if ( isCluster() ) {
            return geometry != null ? geometry.boundingBox.center() : new Vector3Dd();
        }

        Vector3Dd[] p = new Vector3Dd[] {new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()};
        int numberOfVertices = vertices(p);
        Vector3Dd c = new Vector3Dd();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            c = c.add(p[i]);
        }
        if ( numberOfVertices > 0 ) {
            c = c.multiply((double)(1.0 / numberOfVertices));
        }
        return c;
    }

    public void initPolygon(ConvexPolygon polygon) {
        if ( polygon == null ) {
            return;
        }
        if ( isCluster() ) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "galerkinElementPolygon", "Cannot use this function for cluster elements");
            return;
        }
        if ( patch == null ) {
            return;
        }

        polygon.normal = new Vector3Dd(patch.getNormal().x(), patch.getNormal().y(), patch.getNormal().z());
        polygon.planeConstant = patch.getPlaneConstant();
        polygon.numberOfVertices = vertices(polygon.vertex);
        polygon.bounds = new RpkAxisAlignedBoundingBox();
        for ( int i = 0; i < polygon.numberOfVertices; i++ ) {
            polygon.bounds.enlargeToIncludePoint(polygon.vertex[i]);
        }
    }

    public void reAllocCoefficients() {
        byte localBasisSize;

        if ( isCluster() ) {
            localBasisSize = 1;
        }
        else {
            switch ( galerkinState.basisType ) {
                case GALERKIN_CONSTANT:
                    localBasisSize = 1;
                    break;
                case GALERKIN_LINEAR:
                    localBasisSize = 3;
                    break;
                case GALERKIN_QUADRATIC:
                    localBasisSize = 6;
                    break;
                case GALERKIN_CUBIC:
                    localBasisSize = 10;
                    break;
                default:
                    localBasisSize = 1;
                    break;
            }
        }

        ColorRgb[] defaultRadiance = new ColorRgb[localBasisSize];
        ColorRgb[] defaultReceivedRadiance = new ColorRgb[localBasisSize];
        ColorRgb[] defaultUnShotRadiance = new ColorRgb[localBasisSize];
        for ( int i = 0; i < localBasisSize; i++ ) {
            defaultRadiance[i] = new ColorRgb();
            defaultReceivedRadiance[i] = new ColorRgb();
            defaultUnShotRadiance[i] = new ColorRgb();
        }
        ColorRgbMath.arrayClear(defaultRadiance, localBasisSize);
        ColorRgbMath.arrayClear(defaultReceivedRadiance, localBasisSize);
        ColorRgbMath.arrayClear(defaultUnShotRadiance, localBasisSize);

        if ( radiance != null ) {
            ColorRgbMath.arrayCopy(defaultRadiance, radiance, Math.min(basisSize, localBasisSize));
        }
        if ( receivedRadiance != null ) {
            ColorRgbMath.arrayCopy(defaultReceivedRadiance, receivedRadiance, Math.min(basisSize, localBasisSize));
        }
        if ( unShotRadiance != null ) {
            ColorRgbMath.arrayCopy(defaultUnShotRadiance, unShotRadiance, Math.min(basisSize, localBasisSize));
        }

        radiance = defaultRadiance;
        receivedRadiance = defaultReceivedRadiance;
        unShotRadiance = defaultUnShotRadiance;
        basisSize = localBasisSize;
        basisUsed = localBasisSize;
    }

    public Patch getPatch() {
        return patch;
    }

    public void setPatch(Patch inPatch) {
        patch = inPatch;
    }
}
