/**
Galerkin finite elements: one structure for both surface and cluster elements
*/

package vsdk.toolkit.galerkin;

import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.QuadCubatureRule;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.numericalAnalysis.TriangleCubatureRule;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.ElementFlags;
import vsdk.toolkit.skin.ElementTypes;
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

    private static final Matrix2x2[] quadToParentTransformMatrix = buildQuadTransforms();
    private static final Matrix2x2[] triangleToParentTransformMatrix = buildTriangleTransforms();

    private static Matrix2x2[] buildQuadTransforms() {
        Matrix2x2[] t = new Matrix2x2[4];
        for ( int i = 0; i < 4; i++ ) {
            t[i] = new Matrix2x2();
            t[i].m[0][0] = 0.5f;
            t[i].m[0][1] = 0.0f;
            t[i].m[1][0] = 0.0f;
            t[i].m[1][1] = 0.5f;
        }
        t[0].t[0] = 0.0f; t[0].t[1] = 0.0f;
        t[1].t[0] = 0.5f; t[1].t[1] = 0.0f;
        t[2].t[0] = 0.0f; t[2].t[1] = 0.5f;
        t[3].t[0] = 0.5f; t[3].t[1] = 0.5f;
        return t;
    }

    private static Matrix2x2[] buildTriangleTransforms() {
        Matrix2x2[] t = new Matrix2x2[4];
        for ( int i = 0; i < 4; i++ ) {
            t[i] = new Matrix2x2();
        }
        t[0].m[0][0] = 0.5f; t[0].m[0][1] = 0.0f; t[0].m[1][0] = 0.0f;  t[0].m[1][1] = 0.5f;  t[0].t[0] = 0.0f; t[0].t[1] = 0.0f;
        t[1].m[0][0] = 0.5f; t[1].m[0][1] = 0.0f; t[1].m[1][0] = 0.0f;  t[1].m[1][1] = 0.5f;  t[1].t[0] = 0.5f; t[1].t[1] = 0.0f;
        t[2].m[0][0] = 0.5f; t[2].m[0][1] = 0.0f; t[2].m[1][0] = 0.0f;  t[2].m[1][1] = 0.5f;  t[2].t[0] = 0.0f; t[2].t[1] = 0.5f;
        t[3].m[0][0] =-0.5f; t[3].m[0][1] = 0.0f; t[3].m[1][0] = 0.0f;  t[3].m[1][1] =-0.5f;  t[3].t[0] = 0.5f; t[3].t[1] = 0.5f;
        return t;
    }

    private GalerkinElement(GalerkinState inGalerkinState) {
        super();
        numberOfElements++;
        id = numberOfElements;
        className = ElementTypes.ELEMENT_GALERKIN;
        patch = null;
        geometry = null;
        potential = 0.0f;
        receivedPotential = 0.0f;
        unShotPotential = 0.0f;
        directPotential = 0.0f;
        interactions = new ArrayList<>();
        minimumArea = 0.0f;
        blockerSize = 0.0f;
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
        area = inPatch == null ? 0.0f : inPatch.area;
        minimumArea = area;
        blockerSize = 2.0f * (float)Math.sqrt(area / Math.PI);
        numberOfPatches = inPatch == null ? 0 : 1;
        directPotential = inPatch == null ? 0.0f : inPatch.directPotential;

        if ( inPatch != null ) {
            Rd = PatchVisitor.averageNormalAlbedo(inPatch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
            if ( inPatch.material != null && inPatch.material.getEdf() != null ) {
                flags |= ElementFlags.IS_LIGHT_SOURCE_MASK;
                Ed = PatchVisitor.averageEmittance(inPatch, XxdfComponentFlag.DIFFUSE_COMPONENT);
                Ed.scaleInverse((float)Math.PI, Ed);
            }
            inPatch.radianceData = this;
        }

        reAllocCoefficients();
    }

    public GalerkinElement(Geometry inGeometry, GalerkinState inGalerkinState) {
        this(inGalerkinState);
        geometry = inGeometry;
        patch = null;
        flags |= ElementFlags.IS_CLUSTER_MASK;
        Rd.setMonochrome(1.0f);
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

    public static GalerkinElement fromPatch(Patch patch) {
        if ( patch == null || patch.radianceData == null || !(patch.radianceData instanceof GalerkinElement) ) {
            return null;
        }
        return (GalerkinElement)patch.radianceData;
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

    public static int renderMode(RenderOptions renderOptions) {
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
            child.transformToParent = (patch.numberOfVertices == 3)
                ? triangleToParentTransformMatrix[i]
                : quadToParentTransformMatrix[i];
            child.area = 0.25f * area;
            child.blockerSize = 2.0f * (float)Math.sqrt(child.area / Math.PI);
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
        switch ( patch.numberOfVertices ) {
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

    public int vertices(Vector3D[] p) {
        if ( p == null ) {
            return 0;
        }

        if ( isCluster() ) {
            BoundingBox boundingBox = new BoundingBox();
            bounds(boundingBox);
            boundingBox.corners(p);
            return 8;
        }

        if ( patch == null ) {
            return 0;
        }

        Matrix2x2 topTrans = new Matrix2x2();
        Vector2D uv = new Vector2D();
        if ( transformToParent != null ) {
            topTransform(topTrans);
        }

        uv.x = 0.0f; uv.y = 0.0f;
        if ( transformToParent != null ) topTrans.transformPoint2D(uv, uv);
        patch.uniformPoint(uv.x, uv.y, p[0]);

        uv.x = 1.0f; uv.y = 0.0f;
        if ( transformToParent != null ) topTrans.transformPoint2D(uv, uv);
        patch.uniformPoint(uv.x, uv.y, p[1]);

        if ( patch.numberOfVertices == 4 ) {
            uv.x = 1.0f; uv.y = 1.0f;
            if ( transformToParent != null ) topTrans.transformPoint2D(uv, uv);
            patch.uniformPoint(uv.x, uv.y, p[2]);

            uv.x = 0.0f; uv.y = 1.0f;
            if ( transformToParent != null ) topTrans.transformPoint2D(uv, uv);
            patch.uniformPoint(uv.x, uv.y, p[3]);
        }
        else {
            uv.x = 0.0f; uv.y = 1.0f;
            if ( transformToParent != null ) topTrans.transformPoint2D(uv, uv);
            patch.uniformPoint(uv.x, uv.y, p[2]);
            if ( p.length > 3 && p[3] != null ) {
                p[3].set(0.0f, 0.0f, 0.0f);
            }
        }

        return patch.numberOfVertices;
    }

    public BoundingBox bounds(BoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return null;
        }

        if ( isCluster() && geometry != null ) {
            boundingBox.copyFrom(geometry.boundingBox);
            return boundingBox;
        }

        Vector3D[] p = new Vector3D[] {new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()};
        int numberOfVertices = vertices(p);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            boundingBox.enlargeToIncludePoint(p[i]);
        }
        return boundingBox;
    }

    public Vector3D midPoint() {
        if ( isCluster() ) {
            return geometry != null ? geometry.boundingBox.center() : new Vector3D();
        }

        Vector3D[] p = new Vector3D[] {new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()};
        int numberOfVertices = vertices(p);
        Vector3D c = new Vector3D();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            c.addition(c, p[i]);
        }
        if ( numberOfVertices > 0 ) {
            c.scaledCopy((float)(1.0 / numberOfVertices), c);
        }
        return c;
    }

    public void initPolygon(Polygon polygon) {
        if ( polygon == null ) {
            return;
        }
        if ( isCluster() ) {
            Error.fatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
            return;
        }
        if ( patch == null ) {
            return;
        }

        polygon.normal.set(patch.normal.x, patch.normal.y, patch.normal.z);
        polygon.planeConstant = patch.planeConstant;
        polygon.numberOfVertices = vertices(polygon.vertex);
        polygon.bounds = new BoundingBox();
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
        ColorRgb.arrayClear(defaultRadiance, localBasisSize);
        ColorRgb.arrayClear(defaultReceivedRadiance, localBasisSize);
        ColorRgb.arrayClear(defaultUnShotRadiance, localBasisSize);

        if ( radiance != null ) {
            ColorRgb.arrayCopy(defaultRadiance, radiance, Math.min(basisSize, localBasisSize));
        }
        if ( receivedRadiance != null ) {
            ColorRgb.arrayCopy(defaultReceivedRadiance, receivedRadiance, Math.min(basisSize, localBasisSize));
        }
        if ( unShotRadiance != null ) {
            ColorRgb.arrayCopy(defaultUnShotRadiance, unShotRadiance, Math.min(basisSize, localBasisSize));
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
