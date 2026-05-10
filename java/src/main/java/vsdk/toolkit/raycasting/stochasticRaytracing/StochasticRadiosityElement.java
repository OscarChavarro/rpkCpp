/**
Monte Carlo radiosity element type
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.ElementFlags;
import vsdk.toolkit.skin.ElementTypes;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;
import vsdk.toolkit.skin.Vertex;
import vsdk.toolkit.tonemap.ToneMap;

public final class StochasticRadiosityElement extends Element {
    public Patch patch;
    public Geometry geometry;
    public long rayIndex; // Incremented each time a ray is shot from the element
    public float quality; // For merging the result of multiple iterations
    public float samplingProbability;
    public float ng; // Number of samples gathered on the patch

    public GalerkinBasis basis; // Radiosity approximation data
    // Higher order approximations need an array of color values for representing radiance
    public ColorRgb sourceRad; // Always constant source radiosity

    public float importance; // For view-importance driven sampling
    public float unShotImportance;
    public float receivedImportance;
    public float sourceImportance;
    public long importanceRayIndex; // Ray index for importance propagation

    public Vector3D midPoint;
    public Vertex[] vertices; // Up to 4 vertex pointers for surface elements
    public byte childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    public byte numberOfVertices; // Number of surface element vertices

    private static int coefficientPoolsInitialized = 0;
    private static long nextId = 1;

    public StochasticRadiosityElement() {
        super();
        patch = null;
        geometry = null;
        rayIndex = 0L;
        quality = 0.0f;
        samplingProbability = 0.0f;
        ng = 0.0f;
        basis = null;
        sourceRad = new ColorRgb();
        importance = 0.0f;
        unShotImportance = 0.0f;
        receivedImportance = 0.0f;
        sourceImportance = 0.0f;
        importanceRayIndex = 0L;
        midPoint = new Vector3D();
        vertices = new Vertex[4];
        childNumber = -1;
        numberOfVertices = 0;
        className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
    }

    public void destroy() {
    }

    public static boolean coefficientPoolsAreInitialized() {
        return coefficientPoolsInitialized != 0;
    }

    public static void markCoefficientPoolsInitialized() {
        coefficientPoolsInitialized = 1;
    }

    private static void vertexAttachElement(Vertex v, StochasticRadiosityElement elem) {
        elem.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
        if ( v.radianceData == null ) {
            v.radianceData = new ArrayList<>();
        }
        v.radianceData.add(elem);
    }

    private static StochasticRadiosityElement createElement() {
        StochasticRadiosityElement elem = new StochasticRadiosityElement();

        elem.patch = null;
        elem.geometry = null;
        elem.id = (int)nextId;
        nextId++;
        elem.area = 0.0f;
        Coefficientsmcrad.initCoefficients(elem); // Allocation of the coefficients is left until just before the first iteration
        // in Mcrad::monteCarloRadiosityReInit()

        elem.Ed.clear();
        elem.Rd.clear();

        elem.rayIndex = 0L;
        elem.quality = 0;
        elem.ng = 0.0f;

        elem.importance = 0.0f;
        elem.unShotImportance = 0.0f;
        elem.sourceImportance = 0.0f;
        elem.importanceRayIndex = 0L;

        elem.midPoint.set(0.0f, 0.0f, 0.0f);
        elem.vertices[0] = elem.vertices[1] = elem.vertices[2] = elem.vertices[3] = null;
        elem.parent = null;
        elem.regularSubElements = null;
        elem.irregularSubElements = null;
        elem.transformToParent = null;
        elem.childNumber = -1;
        elem.numberOfVertices = 0;
        elem.flags = 0x00;

        ElementHierarchyState.activeState().nr_elements++;

        return elem;
    }

    public static StochasticRadiosityElement stochasticRadiosityElementCreateFromPatch(Patch patch) {
        StochasticRadiosityElement elem = createElement();
        elem.patch = patch;
        elem.flags = 0x00;
        elem.area = patch.area;
        elem.midPoint.copy(patch.midPoint);
        elem.numberOfVertices = patch.numberOfVertices;
        for ( int i = 0; i < elem.numberOfVertices; i++ ) {
            elem.vertices[i] = patch.vertex[i];
            vertexAttachElement(elem.vertices[i], elem);
        }

        Coefficientsmcrad.allocCoefficients(elem); // May need reallocation before the start of the computations
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);

        elem.Ed = PatchVisitor.averageEmittance(patch, XxdfComponentFlag.DIFFUSE_COMPONENT);
        elem.Ed.scaleInverse((float)Math.PI, elem.Ed);
        elem.Rd = PatchVisitor.averageNormalAlbedo(patch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        return elem;
    }

    private static StochasticRadiosityElement monteCarloRadiosityCreateCluster(Geometry geometry) {
        StochasticRadiosityElement elem = createElement();

        elem.geometry = geometry;
        elem.flags = ElementFlags.IS_CLUSTER_MASK;

        elem.Rd.setMonochrome(1.0f);
        elem.Ed.clear();

        // elem->area will be computed from the sub-elements in the cluster later
        elem.midPoint = geometry.boundingBox.center();

        Coefficientsmcrad.allocCoefficients(elem); // Always constant approx. so no need to delay allocating the coefficients
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
        elem.importance = 0.0f;
        elem.unShotImportance = 0.0f;
        elem.receivedImportance = 0.0f;

        ElementHierarchyState.activeState().nr_clusters++;

        return elem;
    }

    private static void monteCarloRadiosityCreateSurfaceElementChild(Patch patch, StochasticRadiosityElement parent) {
        StochasticRadiosityElement elem = (StochasticRadiosityElement)patch.radianceData; // Created before
        elem.parent = parent;

        elem.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
        if ( parent.irregularSubElements == null ) {
            parent.irregularSubElements = new ArrayList<>();
        }
        parent.irregularSubElements.add(elem);
    }

    private static void monteCarloRadiosityCreateClusterChild(Geometry geometry, StochasticRadiosityElement parent) {
        StochasticRadiosityElement subCluster = monteCarloRadiosityCreateClusterHierarchyRecursive(geometry);
        subCluster.parent = parent;
        subCluster.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
        if ( parent.irregularSubElements == null ) {
            parent.irregularSubElements = new ArrayList<>();
        }
        parent.irregularSubElements.add(subCluster);
    }

    /**
Initialises parent cluster radiance/importance/area for child voxelData
*/
    private static void monteCarloRadiosityInitClusterPull(StochasticRadiosityElement parent, StochasticRadiosityElement child) {
        parent.area += child.area;
        StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.radiance, child.radiance);
        StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.unShotRadiance, child.unShotRadiance);
        StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.receivedRadiance, child.receivedRadiance);
        parent.importance += child.area / parent.area * child.importance;
        parent.unShotImportance += child.area / parent.area * child.unShotImportance;
        parent.receivedImportance += child.area / parent.area * child.receivedImportance;

        // Needs division by parent->area once it is known after monteCarloRadiosityInitClusterPull for
        // all children elements
        parent.Ed.addScaled(parent.Ed, child.area, child.Ed);
    }

    private static void monteCarloRadiosityCreateClusterChildren(StochasticRadiosityElement parent) {
        Geometry geometry = parent.geometry;

        if ( geometry.isCompound() ) {
            ArrayList<Geometry> geometryList = Geometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                monteCarloRadiosityCreateClusterChild(geometryList.get(i), parent);
            }
        } else {
            ArrayList<Patch> patchList = Geometry.patchListReference(geometry);
            for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
                monteCarloRadiosityCreateSurfaceElementChild(patchList.get(i), parent);
            }
        }

        for ( int i = 0; parent.irregularSubElements != null && i < parent.irregularSubElements.size(); i++ ) {
            monteCarloRadiosityInitClusterPull(parent, (StochasticRadiosityElement)parent.irregularSubElements.get(i));
        }
        parent.Ed.scaleInverse(parent.area, parent.Ed);
    }

    private static StochasticRadiosityElement monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry world) {
        StochasticRadiosityElement topCluster = monteCarloRadiosityCreateCluster(world);
        world.radianceData = topCluster;
        monteCarloRadiosityCreateClusterChildren(topCluster);
        return topCluster;
    }

    public static StochasticRadiosityElement stochasticRadiosityElementCreateFromGeometry(Geometry world) {
        if ( world == null ) {
            return null;
        } else {
            return monteCarloRadiosityCreateClusterHierarchyRecursive(world);
        }
    }

    /**
Determine the (u, v) coordinate range of the element w.r.t. the patch to
which it belongs when using regular quadtree subdivision in
order to efficiently generate samples with Niederreiter::NextNiedInRange()
in the Niederreiter core implementation. Niederreiter::NextNiedInRange() creates a sample on a quadrilateral
subdomain, called a "dyadic box" in QMC literature. All samples in
such a dyadic box have the same most significant bits. This routine
basically computes what these most significant bits are. The computation
is based on the regular quadtree subdivision of a quadrilateral, as
shown above. If a triangular element is to be sampled, the quadrilateral
sample needs to be "folded" into the triangle. FoldSample() in sample4d.c
does this
*/
    public static void stochasticRadiosityElementRange(
        StochasticRadiosityElement elem,
        int[] numberOfBits,
        long[] mostSignificantBits1,
        long[] rMostSignificantBits2)
    {
        int nb = 0;
        long b1 = 0;
        long b2 = 0;
        while ( elem.childNumber >= 0 ) {
            nb++;
            b1 = (b1 << 1) | (elem.childNumber & 1);
            b2 = (b2 >> 1) | ((elem.childNumber & 2) << (Niederreiter.NBITS - 2));
            elem = (StochasticRadiosityElement)elem.parent;
        }

        numberOfBits[0] = nb;
        mostSignificantBits1[0] = b1;
        rMostSignificantBits2[0] = b2;
    }

    /**
Determines the regular sub-element at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
    public static StochasticRadiosityElement stochasticRadiosityElementRegularSubElementAtPoint(
        StochasticRadiosityElement parent,
        double[] u,
        double[] v)
    {
        StochasticRadiosityElement child = null;
        double _u = u[0];
        double _v = v[0];

        if ( parent.isCluster() || parent.regularSubElements == null ) {
            return null;
        }

        // Have a look at the drawings above to understand what is done exactly
        switch ( parent.numberOfVertices ) {
            case 3:
                if ( _u + _v <= 0.5 ) {
                    child = (StochasticRadiosityElement)parent.regularSubElements[0];
                    u[0] = _u * 2.0;
                    v[0] = _v * 2.0;
                } else if ( _u > 0.5 ) {
                    child = (StochasticRadiosityElement)parent.regularSubElements[1];
                    u[0] = (_u - 0.5) * 2.0;
                    v[0] = _v * 2.0;
                } else if ( _v > 0.5 ) {
                    child = (StochasticRadiosityElement)parent.regularSubElements[2];
                    u[0] = _u * 2.0;
                    v[0] = (_v - 0.5) * 2.0;
                } else {
                    child = (StochasticRadiosityElement)parent.regularSubElements[3];
                    u[0] = (0.5 - _u) * 2.0;
                    v[0] = (0.5 - _v) * 2.0;
                }
                break;
            case 4:
                if ( _v <= 0.5 ) {
                    if ( _u < 0.5 ) {
                        child = (StochasticRadiosityElement)parent.regularSubElements[0];
                        u[0] = _u * 2.0;
                    } else {
                        child = (StochasticRadiosityElement)parent.regularSubElements[1];
                        u[0] = (_u - 0.5) * 2.0;
                    }
                    v[0] = _v * 2.0;
                } else {
                    if ( _u < 0.5 ) {
                        child = (StochasticRadiosityElement)parent.regularSubElements[2];
                        u[0] = _u * 2.0;
                    } else {
                        child = (StochasticRadiosityElement)parent.regularSubElements[3];
                        u[0] = (_u - 0.5) * 2.0;
                    }
                    v[0] = (_v - 0.5) * 2.0;
                }
                break;
            default:
                Logger.fatal(-1, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
        }

        return child;
    }

    /**
Returns the leaf regular sub-element of 'top' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'top' is a surface element, not a cluster
*/
    public static StochasticRadiosityElement stochasticRadiosityElementRegularLeafElementAtPoint(StochasticRadiosityElement top, double[] u, double[] v) {
        StochasticRadiosityElement leaf;

        // Find leaf element of 'top' at (u,v)
        leaf = top;
        while ( leaf.regularSubElements != null ) {
            leaf = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(leaf, u, v);
        }

        return leaf;
    }

    private static Vector3D monteCarloRadiosityInstallCoordinate(Vector3D coord) {
        Vector3D v = new Vector3D(coord.x, coord.y, coord.z);
        ElementHierarchyState.activeState().coords.add(v);
        return v;
    }

    private static Vector3D monteCarloRadiosityInstallNormal(Vector3D normal) {
        Vector3D v = new Vector3D(normal.x, normal.y, normal.z);
        ElementHierarchyState.activeState().normals.add(v);
        return v;
    }

    private static Vector3D monteCarloRadiosityInstallTexCoord(Vector3D texCoord) {
        Vector3D t = new Vector3D(texCoord.x, texCoord.y, texCoord.z);
        ElementHierarchyState.activeState().texCoords.add(t);
        return t;
    }

    private static Vertex monteCarloRadiosityInstallVertex(Vector3D coord, Vector3D normal, Vector3D texCoord) {
        ArrayList<Patch> newPatchList = new ArrayList<>();
        Vertex v = new Vertex(coord, normal, texCoord, newPatchList);
        ElementHierarchyState.activeState().vertices.add(v);
        return v;
    }

    private static Vertex monteCarloRadiosityNewMidpointVertex(StochasticRadiosityElement elem, Vertex v1, Vertex v2) {
        Vector3D coord = new Vector3D();
        Vector3D norm = new Vector3D();
        Vector3D texCoord = new Vector3D();
        Vector3D p;
        Vector3D n;
        Vector3D t;

        coord.midPoint(v1.point, v2.point);
        p = monteCarloRadiosityInstallCoordinate(coord);

        if ( v1.normal != null && v2.normal != null ) {
            norm.midPoint(v1.normal, v2.normal);
            n = monteCarloRadiosityInstallNormal(norm);
        } else {
            n = elem.patch.normal;
        }

        if ( v1.textureCoordinates != null && v2.textureCoordinates != null ) {
            texCoord.midPoint(v1.textureCoordinates, v2.textureCoordinates);
            t = monteCarloRadiosityInstallTexCoord(texCoord);
        } else {
            t = null;
        }

        return monteCarloRadiosityInstallVertex(p, n, t);
    }

    /**
Finds the surface element adjacent to 'elem' across the edge with index
'edgeNumber'. That edge is defined by:
  elem->vertices[edgeNumber]
  elem->vertices[(edgeNumber + 1) % elem->numberOfVertices]
The method searches the stochastic radiosity elements attached to the second
vertex and returns the first element (different from 'elem') that contains the
same edge with opposite orientation. Returns nullptr when no neighbour is
found.
*/
    private static StochasticRadiosityElement monteCarloRadiosityElementNeighbour(StochasticRadiosityElement elem, int edgeNumber) {
        Vertex from = elem.vertices[edgeNumber];
        Vertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];

        for ( int i = 0; to.radianceData != null && i < to.radianceData.size(); i++ ) {
            Element element = to.radianceData.get(i);
            if ( element.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement e = (StochasticRadiosityElement)element;
            if ( e != elem &&
                 ((e.numberOfVertices == 3 &&
                   ((e.vertices[0] == to && e.vertices[1] == from) ||
                    (e.vertices[1] == to && e.vertices[2] == from) ||
                    (e.vertices[2] == to && e.vertices[0] == from)))
                  || (e.numberOfVertices == 4 &&
                      ((e.vertices[0] == to && e.vertices[1] == from) ||
                       (e.vertices[1] == to && e.vertices[2] == from) ||
                       (e.vertices[2] == to && e.vertices[3] == from) ||
                       (e.vertices[3] == to && e.vertices[0] == from)))) ) {
                return e;
            }
        }

        return null;
    }

    public static Vertex stochasticRadiosityElementEdgeMidpointVertex(StochasticRadiosityElement elem, int edgeNumber) {
        Vertex v = null;
        Vertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];
        StochasticRadiosityElement neighbour = monteCarloRadiosityElementNeighbour(elem, edgeNumber);

        if ( neighbour != null && neighbour.regularSubElements != null ) {
            int index;

            if ( to == neighbour.vertices[0] ) {
                index = 0;
            } else if ( to == neighbour.vertices[1] ) {
                index = 1;
            } else if ( to == neighbour.vertices[2] ) {
                index = 2;
            } else if ( to == neighbour.vertices[3] ) {
                index = 3;
            } else {
                index = -1;
            }

            switch ( neighbour.numberOfVertices ) {
                case 3:
                    switch ( index ) {
                        case 0:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[0]).vertices[1];
                            break;
                        case 1:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[1]).vertices[2];
                            break;

                        case 2:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[2]).vertices[0];
                            break;
                        default:
                            Logger.error("EdgeMidpointVertex", "Invalid vertex index %d", index);
                    }
                    break;
                case 4:
                    switch ( index ) {
                        case 0:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[0]).vertices[1];
                            break;
                        case 1:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[1]).vertices[2];
                            break;
                        case 2:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[3]).vertices[3];
                            break;
                        case 3:
                            v = ((StochasticRadiosityElement)neighbour.regularSubElements[2]).vertices[0];
                            break;
                        default:
                            Logger.error("EdgeMidpointVertex", "Invalid vertex index %d", index);
                    }
                    break;
                default:
                    Logger.fatal(-1, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
            }
        }

        return v;
    }

    private static Vertex monteCarloRadiosityNewEdgeMidpointVertex(StochasticRadiosityElement elem, int edgeNumber) {
        Vertex v = StochasticRadiosityElement.stochasticRadiosityElementEdgeMidpointVertex(elem, edgeNumber);
        if ( v == null ) {
            // First time we split the edge, create the midpoint vertex
            Vertex from = elem.vertices[edgeNumber];
            Vertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];
            v = monteCarloRadiosityNewMidpointVertex(elem, from, to);
        }
        return v;
    }

    private static Vector3D galerkinElementMidpoint(StochasticRadiosityElement elem) {
        elem.midPoint.set(0.0f, 0.0f, 0.0f);
        for ( int i = 0; i < elem.numberOfVertices; i++ ) {
            elem.midPoint.addition(elem.midPoint, elem.vertices[i].point);
        }
        elem.midPoint.inverseScaledCopy(elem.numberOfVertices, elem.midPoint, Numeric.EPSILON_FLOAT);

        return elem.midPoint;
    }

    /**
Only for surface elements
*/
    public static boolean stochasticRadiosityElementIsTextured(StochasticRadiosityElement elem) {
        if ( elem.isCluster() ) {
            Logger.fatal(-1, "stochasticRadiosityElementIsTextured", "this routine should not be called for cluster elements");
            return false;
        }
        Material mat = elem.patch.material;
        return mat.getBsdf() != null
            && (mat.getBsdf().splitBsdfIsTextured() || PhongEmittanceDistributionFunction.edfIsTextured());
    }

    /**
Uses elem->Rd for surface elements
*/
    public static float stochasticRadiosityElementScalarReflectance(StochasticRadiosityElement elem) {
        float rd;

        if ( elem.isCluster() ) {
            return 1.0f;
        }

        rd = elem.Rd.maximumComponent();
        if ( rd < Numeric.EPSILON ) {
            // Avoid divisions by zero
            rd = Numeric.EPSILON_FLOAT;
        }
        return rd;
    }

    /**
Computes average reflectance and emittance of a surface sub-element
*/
    private static void monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(StochasticRadiosityElement elem) {
        Patch patch = elem.patch;
        int numberOfSamples;
        boolean isTextured;
        int[] nbits = new int[1];
        long[] msb1 = new long[1];
        long[] rMostSignificantBit2 = new long[1];
        long[] n = new long[] {1};
        ColorRgb albedo = new ColorRgb();
        ColorRgb emittance = new ColorRgb();
        RayHit hit = new RayHit();
        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        isTextured = StochasticRadiosityElement.stochasticRadiosityElementIsTextured(elem);
        numberOfSamples = isTextured ? 100 : 1;
        albedo.clear();
        emittance.clear();
        StochasticRadiosityElement.stochasticRadiosityElementRange(elem, nbits, msb1, rMostSignificantBit2);

        for ( int i = 0; i < numberOfSamples; i++, n[0]++ ) {
            ColorRgb sample;
            long[] xi = Niederreiter.NextNiedInRange(n, +1, nbits[0], msb1[0], rMostSignificantBit2[0]);
            hit.setUv((double)xi[0] * Niederreiter.RECIP, (double)xi[1] * Niederreiter.RECIP);
            int newFlags = hit.getFlags() | RayHitFlag.UV;
            hit.setFlags(newFlags);
            Vector3D position = hit.getPoint();
            patch.uniformPoint(hit.getUv().u, hit.getUv().v, position);
            if ( patch.material.getBsdf() != null ) {
                sample = patch.material.getBsdf().splitBsdfScatteredPower(hit, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
                albedo.add(albedo, sample);
            }
            if ( patch.material.getEdf() != null ) {
                sample = patch.material.getEdf().phongEmittance(hit, XxdfComponentFlag.DIFFUSE_COMPONENT);
                emittance.add(emittance, sample);
            }
        }
        elem.Rd.scaleInverse(numberOfSamples, albedo);
        elem.Ed.scaleInverse(numberOfSamples, emittance);
    }

    /**
Initial push operation for surface sub-elements
*/
    private static void monteCarloRadiosityInitSurfacePush(StochasticRadiosityElement parent, StochasticRadiosityElement child) {
        child.sourceRad = new ColorRgb(parent.sourceRad.r, parent.sourceRad.g, parent.sourceRad.b);
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.radiance, child.radiance);
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.unShotRadiance, child.unShotRadiance);

        float[] parentImportance = new float[] {parent.importance};
        float[] childImportance = new float[] {child.importance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentImportance, childImportance);
        child.importance = childImportance[0];

        float[] parentSourceImportance = new float[] {parent.sourceImportance};
        float[] childSourceImportance = new float[] {child.sourceImportance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentSourceImportance, childSourceImportance);
        child.sourceImportance = childSourceImportance[0];

        float[] parentUnShotImportance = new float[] {parent.unShotImportance};
        float[] childUnShotImportance = new float[] {child.unShotImportance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentUnShotImportance, childUnShotImportance);
        child.unShotImportance = childUnShotImportance[0];

        child.rayIndex = parent.rayIndex;
        child.quality = parent.quality;
        child.samplingProbability = parent.samplingProbability * child.area / parent.area;

        child.Rd = new ColorRgb(parent.Rd.r, parent.Rd.g, parent.Rd.b);
        child.Ed = new ColorRgb(parent.Ed.r, parent.Ed.g, parent.Ed.b);
        monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(child);
    }

    /**
Creates a sub-element of the element "*parent", stores it as the
sub-element number "childNumber". Tha value of "v3" is unused in the
process of triangle subdivision.
*/
    private static StochasticRadiosityElement monteCarloRadiosityCreateSurfaceSubElement(
        StochasticRadiosityElement parent,
        int childNumber,
        Vertex v0,
        Vertex v1,
        Vertex v2,
        Vertex v3)
    {
        StochasticRadiosityBasisState basisState = StochasticRadiosityBasisState.activeState();
        StochasticRadiosityElement elem = createElement();
        parent.regularSubElements[childNumber] = elem;

        elem.patch = parent.patch;
        elem.numberOfVertices = parent.numberOfVertices;
        elem.vertices[0] = v0;
        elem.vertices[1] = v1;
        elem.vertices[2] = v2;
        elem.vertices[3] = v3;
        for ( int i = 0; i < elem.numberOfVertices; i++ ) {
            vertexAttachElement(elem.vertices[i], elem);
        }

        elem.area = 0.25f * parent.area; // Regular elements, regular subdivision
        elem.midPoint = galerkinElementMidpoint(elem);

        elem.parent = parent;
        elem.childNumber = (byte)childNumber;
        elem.transformToParent = elem.numberOfVertices == 3
            ? basisState.triangleUpTransform[childNumber]
            : basisState.quadUpTransform[childNumber];

        Coefficientsmcrad.allocCoefficients(elem);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
        elem.importance = elem.unShotImportance = elem.receivedImportance = 0.0f;
        monteCarloRadiosityInitSurfacePush(parent, elem);

        return elem;
    }

    /**
Create sub-elements: regular subdivision, see drawings above
*/
    private static StochasticRadiosityElement[] monteCarloRadiosityRegularSubdivideTriangle(StochasticRadiosityElement element, RenderOptions renderOptions) {
        Vertex v0 = element.vertices[0];
        Vertex v1 = element.vertices[1];
        Vertex v2 = element.vertices[2];
        Vertex m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
        Vertex m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
        Vertex m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);

        monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, m2, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 2, m2, m1, v2, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 3, m1, m2, m0, null);

        return castElementArray(element.regularSubElements);
    }

    private static StochasticRadiosityElement[] monteCarloRadiosityRegularSubdivideQuad(StochasticRadiosityElement element, RenderOptions renderOptions) {
        Vertex v0 = element.vertices[0];
        Vertex v1 = element.vertices[1];
        Vertex v2 = element.vertices[2];
        Vertex v3 = element.vertices[3];
        Vertex m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
        Vertex m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
        Vertex m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);
        Vertex m3 = monteCarloRadiosityNewEdgeMidpointVertex(element, 3);
        Vertex mm = monteCarloRadiosityNewMidpointVertex(element, m0, m2);

        monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, mm, m3);
        monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, mm);
        monteCarloRadiosityCreateSurfaceSubElement(element, 2, m3, mm, m2, v3);
        monteCarloRadiosityCreateSurfaceSubElement(element, 3, mm, m1, v2, m2);

        return castElementArray(element.regularSubElements);
    }

    /**
Subdivides given triangle or quadrangle into four sub-elements if not yet
done so before. Returns the list of created sub-elements
*/
    public static StochasticRadiosityElement[] stochasticRadiosityElementRegularSubdivideElement(
        StochasticRadiosityElement element,
        RenderOptions renderOptions)
    {
        if ( element.regularSubElements != null ) {
            return castElementArray(element.regularSubElements);
        }

        if ( element.isCluster() ) {
            Logger.fatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
            return null;
        }

        if ( element.patch.jacobian != null ) {
            Logger.warning("galerkinElementRegularSubDivide",
                "irregular quadrilateral patches are not correctly handled (but you probably will not notice it)");
        }

        // Create the sub-elements
        element.regularSubElements = new Element[4];
        switch ( element.numberOfVertices ) {
            case 3:
                monteCarloRadiosityRegularSubdivideTriangle(element, renderOptions);
                break;
            case 4:
                monteCarloRadiosityRegularSubdivideQuad(element, renderOptions);
                break;
            default:
                Logger.fatal(-1, "galerkinElementRegularSubDivide", "invalid element: not 3 or 4 vertices");
        }
        return castElementArray(element.regularSubElements);
    }

    private static void monteCarloRadiosityDestroyElement(StochasticRadiosityElement elem) {
        if ( elem == null ) {
            return;
        }
        if ( elem.isCluster() ) {
            ElementHierarchyState.activeState().nr_clusters--;
        }
        ElementHierarchyState.activeState().nr_elements--;

        if ( elem.irregularSubElements != null ) {
            elem.irregularSubElements.clear();
            elem.irregularSubElements = null;
        }

        if ( elem.regularSubElements != null ) {
            elem.regularSubElements = null;
        }

        Coefficientsmcrad.disposeCoefficients(elem);
    }

    private static void monteCarloRadiosityDestroySurfaceElement(StochasticRadiosityElement elem) {
        if ( elem == null ) {
            return;
        }
        if ( elem.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                monteCarloRadiosityDestroySurfaceElement((StochasticRadiosityElement)elem.regularSubElements[i]);
            }
        }
        monteCarloRadiosityDestroyElement(elem);
    }

    public static void stochasticRadiosityElementDestroy(StochasticRadiosityElement elem) {
        monteCarloRadiosityDestroySurfaceElement(elem);
    }

    public static void stochasticRadiosityElementDestroyClusterHierarchy(StochasticRadiosityElement top) {
        if ( top == null || !top.isCluster() ) {
            return;
        }
        for ( int i = 0; top.irregularSubElements != null && i < top.irregularSubElements.size(); i++ ) {
            StochasticRadiosityElement element = (StochasticRadiosityElement)top.irregularSubElements.get(i);
            if ( element.isCluster() ) {
                StochasticRadiosityElement.stochasticRadiosityElementDestroyClusterHierarchy(element);
            }
        }
        monteCarloRadiosityDestroyElement(top);
    }

    private static boolean regularChild(StochasticRadiosityElement child) {
        return (child.childNumber >= 0 && child.childNumber <= 3);
    }

    public static void stochasticRadiosityElementPushRadiance(
        StochasticRadiosityElement parent,
        StochasticRadiosityElement child,
        ColorRgb[] parentRadiance,
        ColorRgb[] childRadiance)
    {
        if ( parent.isCluster() || child.basis.size == 1 ) {
            childRadiance[0].add(childRadiance[0], parentRadiance[0]);
        } else if ( regularChild(child) && child.basis == parent.basis ) {
            Basismcrad.filterColorDown(parentRadiance, child.basis.regularFilter[child.childNumber], childRadiance,
                child.basis.size);
        } else {
            Logger.fatal(-1, "stochasticRadiosityElementPushRadiance",
                "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
        }
    }

    public static void stochasticRadiosityElementPushImportance(float[] parentImportance, float[] childImportance) {
        childImportance[0] += parentImportance[0];
    }

    public static void stochasticRadiosityElementPullRadiance(
        StochasticRadiosityElement parent,
        StochasticRadiosityElement child,
        ColorRgb[] parentRad,
        ColorRgb[] childRad)
    {
        float areaFactor = child.area / parent.area;
        if ( parent.isCluster() || child.basis.size == 1 ) {
            parentRad[0].addScaled(parentRad[0], areaFactor, childRad[0]);
        } else if ( regularChild(child) && child.basis == parent.basis ) {
            Basismcrad.filterColorUp(childRad, child.basis.regularFilter[child.childNumber],
                parentRad, child.basis.size, areaFactor);
        } else {
            Logger.fatal(-1, "stochasticRadiosityElementPullRadiance",
                "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
        }
    }

    public static void stochasticRadiosityElementPullImportance(StochasticRadiosityElement parent, StochasticRadiosityElement child, float[] parentImportance, float[] childImportance) {
        parentImportance[0] += child.area / parent.area * childImportance[0];
    }

    public static ColorRgb stochasticRadiosityElementColor(StochasticRadiosityElement element) {
        ColorRgb color = new ColorRgb();

        switch ( StochasticRelaxation.activeState().show ) {
            case SHOW_TOTAL_RADIANCE:
            case SHOW_INDIRECT_RADIANCE:
                ToneMap.radianceToRgb(
                    StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(element),
                    color,
                    StochasticRelaxation.activeState().toneMapOptions);
                break;
            case SHOW_IMPORTANCE: {
                float gray;

                if ( element.importance > 1.0 ) {
                    gray = 1.0f;
                } else {
                    gray = element.importance < 0.0 ? 0.0f : element.importance;
                }

                color.set(gray, gray, gray);
                break;
            }
            default:
                Logger.fatal(
                    -1,
                    "stochasticRadiosityElementColor",
                    "Do not know what to display (StochasticRelaxation::activeState().show = %d)",
                    StochasticRelaxation.activeState().show.ordinal());
        }

        return color;
    }

    private static ColorRgb vertexRadiance(Vertex v) {
        int count = 0;
        ColorRgb radiance = new ColorRgb();

        radiance.clear();
        for ( int i = 0; v.radianceData != null && i < v.radianceData.size(); i++ ) {
            Element element = v.radianceData.get(i);
            if ( element.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement elem = (StochasticRadiosityElement)element;
            if ( elem.regularSubElements == null ) {
                ColorRgb elementRadiosity = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
                radiance.add(radiance, elementRadiosity);
                count++;
            }
        }

        if ( count > 0 ) {
            radiance.scaleInverse(count, radiance);
        }

        return radiance;
    }

    /**
Same as above but for importance
*/
    private static float vertexImportance(Vertex v) {
        int count = 0;
        float imp = 0.0f;

        for ( int i = 0; v.radianceData != null && i < v.radianceData.size(); i++ ) {
            Element genericElement = v.radianceData.get(i);
            if ( genericElement.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement element = (StochasticRadiosityElement)genericElement;
            if ( element.regularSubElements == null ) {
                imp += element.importance;
                count++;
            }
        }

        if ( count > 0 ) {
            imp /= (float)count;
        }

        return imp;
    }

    private static ColorRgb vertexColor(Vertex v) {
        switch ( StochasticRelaxation.activeState().show ) {
            case SHOW_TOTAL_RADIANCE:
            case SHOW_INDIRECT_RADIANCE:
                ToneMap.radianceToRgb(
                    vertexRadiance(v),
                    v.color,
                    StochasticRelaxation.activeState().toneMapOptions);
                break;
            case SHOW_IMPORTANCE: {
                float gray = vertexImportance(v);
                if ( gray > 1.0 ) {
                    gray = 1.0f;
                }
                if ( gray < 0.0 ) {
                    gray = 0.0f;
                }
                v.color.set(gray, gray, gray);
                break;
            }
            default:
                Logger.fatal(-1, "vertexColor",
                    "Do not know what to display (StochasticRelaxation::activeState().show = %d)",
                    StochasticRelaxation.activeState().show.ordinal());
        }

        return v.color;
    }

    /**
Compute new vertex colors
*/
    public static void stochasticRadiosityElementComputeNewVertexColors(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;
        vertexColor(stochasticRadiosityElement.vertices[0]);
        vertexColor(stochasticRadiosityElement.vertices[1]);
        vertexColor(stochasticRadiosityElement.vertices[2]);
        if ( stochasticRadiosityElement.numberOfVertices > 3 ) {
            vertexColor(stochasticRadiosityElement.vertices[3]);
        }
    }

    public static void stochasticRadiosityElementAdjustTVertexColors(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;
        Vertex[] m = new Vertex[4];
        int n = 0;
        for ( int i = 0; i < stochasticRadiosityElement.numberOfVertices; i++ ) {
            m[i] = StochasticRadiosityElement.stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
            if ( m[i] != null ) {
                n++;
            }
        }

        if ( n > 0 ) {
            ColorRgb color = StochasticRadiosityElement.stochasticRadiosityElementColor(stochasticRadiosityElement);
            for ( int i = 0; i < stochasticRadiosityElement.numberOfVertices; i++ ) {
                if ( m[i] != null ) {
                    m[i].color.r = (m[i].color.r + color.r) * 0.5f;
                    m[i].color.g = (m[i].color.g + color.g) * 0.5f;
                    m[i].color.b = (m[i].color.b + color.b) * 0.5f;
                }
            }
        }
    }

    public static ColorRgb stochasticRadiosityElementDisplayRadiance(StochasticRadiosityElement elem) {
        ColorRgb radiance = new ColorRgb();
        radiance.subtract(elem.radiance[0], elem.sourceRad);

        if ( StochasticRelaxation.activeState().show != WhatToShow.SHOW_INDIRECT_RADIANCE ) {
            // sourceRad is self-emitted radiance when indirect-only is disabled.
            // Otherwise it represents direct illumination.
            radiance.add(radiance, elem.sourceRad);
            if ( StochasticRelaxation.activeState().indirectOnly != 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot != 0 ) {
                // Add self-emitted radiance
                radiance.add(radiance, elem.Ed);
            }
        }
        return radiance;
    }

    public static ColorRgb stochasticRadiosityElementDisplayRadianceAtPoint(
        StochasticRadiosityElement elem,
        double u,
        double v,
        RenderOptions renderOptions)
    {
        ColorRgb radiance = new ColorRgb();
        if ( elem.basis.size == 1 ) {
            if ( renderOptions.smoothShading ) {
                // Do Gouraud interpolation if required
                ColorRgb[] rad = new ColorRgb[4];
                for ( int i = 0; i < elem.numberOfVertices; i++ ) {
                    rad[i] = vertexRadiance(elem.vertices[i]);
                }
                switch ( elem.numberOfVertices ) {
                    case 3:
                        radiance.interpolateBarycentric(rad[0], rad[1], rad[2], (float)u, (float)v);
                        break;
                    case 4:
                        radiance.interpolateBiLinear(rad[0], rad[1], rad[2], rad[3], (float)u, (float)v);
                        break;
                    default:
                        Logger.fatal(-1, "stochasticRadiosityElementDisplayRadianceAtPoint",
                            "can only handle triangular or quadrilateral elements");
                }
            } else {
                // Flat shading
                radiance = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
            }
        } else {
            // Higher order approximations
            radiance = Basismcrad.colorAtUv(elem.basis, elem.radiance, u, v);
            if ( StochasticRelaxation.activeState().show == WhatToShow.SHOW_INDIRECT_RADIANCE ) {
                radiance.subtract(radiance, elem.sourceRad);
            }
        }
        return radiance;
    }

    public Patch getPatch() {
        return patch;
    }

    public void setPatch(Patch inPatch) {
        patch = inPatch;
    }

    public Geometry getGeometry() {
        return geometry;
    }

    public void setGeometry(Geometry inGeometry) {
        geometry = inGeometry;
    }

    private static StochasticRadiosityElement[] castElementArray(Element[] array) {
        StochasticRadiosityElement[] out = new StochasticRadiosityElement[array.length];
        for ( int i = 0; i < array.length; i++ ) {
            out[i] = (StochasticRadiosityElement)array[i];
        }
        return out;
    }
}
