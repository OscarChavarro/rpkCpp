/**
Monte Carlo radiosity element type
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;
import vsdk.toolkit.tonemap.ToneMap;

public final class StochasticRadiosityElement extends Element {
    public Patch patch;
    public RpkGeometry geometry;
    public long rayIndex; // Incremented each time a ray is shot from the element
    public double quality; // For merging the result of multiple iterations
    public double samplingProbability;
    public double ng; // Number of samples gathered on the patch

    public StochasticRadiosityBasis basis; // Radiosity approximation data
    // Higher order approximations need an array of color values for representing radiance
    public ColorRgb sourceRad; // Always constant source radiosity

    public double importance; // For view-importance driven sampling
    public double unShotImportance;
    public double receivedImportance;
    public double sourceImportance;
    public long importanceRayIndex; // Ray index for importance propagation

    public Vector3Dd midPoint;
    public RpkVertex[] vertices; // Up to 4 vertex pointers for surface elements
    public byte childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    public byte numberOfVertices; // Number of surface element vertices

    private static int coefficientPoolsInitialized = 0;
    private static long nextId = 1;

    public StochasticRadiosityElement() {
        super();
        patch = null;
        geometry = null;
        rayIndex = 0L;
        quality = 0.0;
        samplingProbability = 0.0;
        ng = 0.0;
        basis = null;
        sourceRad = new ColorRgb();
        importance = 0.0;
        unShotImportance = 0.0;
        receivedImportance = 0.0;
        sourceImportance = 0.0;
        importanceRayIndex = 0L;
        midPoint = new Vector3Dd();
        vertices = new RpkVertex[4];
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

    private static void vertexAttachElement(RpkVertex v, StochasticRadiosityElement elem) {
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
        elem.area = 0.0;
        Coefficientsmcrad.initCoefficients(elem); // Allocation of the coefficients is left until just before the first iteration
        // in Mcrad::monteCarloRadiosityReInit()

        elem.Ed = new ColorRgb(0.0, 0.0, 0.0);
        elem.Rd = new ColorRgb(0.0, 0.0, 0.0);

        elem.rayIndex = 0L;
        elem.quality = 0;
        elem.ng = 0.0;

        elem.importance = 0.0;
        elem.unShotImportance = 0.0;
        elem.sourceImportance = 0.0;
        elem.importanceRayIndex = 0L;

        elem.midPoint = new Vector3Dd(0.0, 0.0, 0.0);
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
        elem.area = patch.getArea();
        elem.midPoint = patch.midPoint();
        elem.numberOfVertices = (byte)patch.getNumberOfVertices();
        for ( int i = 0; i < elem.numberOfVertices; i++ ) {
            elem.vertices[i] = patch.getVertices()[i];
            vertexAttachElement(elem.vertices[i], elem);
        }

        Coefficientsmcrad.allocCoefficients(elem); // May need reallocation before the start of the computations
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);

        elem.Ed = PatchVisitor.averageEmittance(patch, XxdfComponentFlag.DIFFUSE_COMPONENT);
        elem.Ed = elem.Ed.scaleInverse((double)Math.PI);
        elem.Rd = PatchVisitor.averageNormalAlbedo(patch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        return elem;
    }

    private static StochasticRadiosityElement monteCarloRadiosityCreateCluster(RpkGeometry geometry) {
        StochasticRadiosityElement elem = createElement();

        elem.geometry = geometry;
        elem.flags = ElementFlags.IS_CLUSTER_MASK;

        elem.Rd = new ColorRgb(1.0, 1.0, 1.0);
        elem.Ed = new ColorRgb(0.0, 0.0, 0.0);

        // elem->area will be computed from the sub-elements in the cluster later
        elem.midPoint = geometry.boundingBox.center();

        Coefficientsmcrad.allocCoefficients(elem); // Always constant approx. so no need to delay allocating the coefficients
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
        elem.importance = 0.0;
        elem.unShotImportance = 0.0;
        elem.receivedImportance = 0.0;

        ElementHierarchyState.activeState().nr_clusters++;

        return elem;
    }

    private static void monteCarloRadiosityCreateSurfaceElementChild(Patch patch, StochasticRadiosityElement parent) {
        StochasticRadiosityElement elem = McradP.topLevelStochasticRadiosityElement(patch); // Created before
        elem.parent = parent;

        elem.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
        if ( parent.irregularSubElements == null ) {
            parent.irregularSubElements = new ArrayList<>();
        }
        parent.irregularSubElements.add(elem);
    }

    private static void monteCarloRadiosityCreateClusterChild(RpkGeometry geometry, StochasticRadiosityElement parent) {
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
        parent.Ed = parent.Ed.addScaled(child.area, child.Ed);
    }

    private static void monteCarloRadiosityCreateClusterChildren(StochasticRadiosityElement parent) {
        RpkGeometry geometry = parent.geometry;

        if ( geometry.isCompound() ) {
            ArrayList<RpkGeometry> geometryList = RpkGeometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                monteCarloRadiosityCreateClusterChild(geometryList.get(i), parent);
            }
        } else {
            ArrayList<Patch> patchList = RpkGeometry.patchListReference(geometry);
            for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
                monteCarloRadiosityCreateSurfaceElementChild(patchList.get(i), parent);
            }
        }

        for ( int i = 0; parent.irregularSubElements != null && i < parent.irregularSubElements.size(); i++ ) {
            monteCarloRadiosityInitClusterPull(parent, (StochasticRadiosityElement)parent.irregularSubElements.get(i));
        }
        parent.Ed = parent.Ed.scaleInverse(parent.area);
    }

    private static StochasticRadiosityElement monteCarloRadiosityCreateClusterHierarchyRecursive(RpkGeometry world) {
        StochasticRadiosityElement topCluster = monteCarloRadiosityCreateCluster(world);
        world.radianceData = topCluster;
        monteCarloRadiosityCreateClusterChildren(topCluster);
        return topCluster;
    }

    public static StochasticRadiosityElement stochasticRadiosityElementCreateFromGeometry(RpkGeometry world) {
        if ( world == null ) {
            return null;
        } else {
            return monteCarloRadiosityCreateClusterHierarchyRecursive(world);
        }
    }

    /**
Determine the (u, v) coordinate range of the element w.getR().t. the patch to
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
            b1 = (b1 << 1) | (long) (elem.childNumber & 1);
            b2 = (b2 >>> 1) | ((long) (elem.childNumber & 2) << (Niederreiter.NBITS - 2));
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
                Logger.reportMessage(null, Logger.FATAL_ERROR, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
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

    private static Vector3Dd monteCarloRadiosityInstallCoordinate(Vector3Dd coord) {
        Vector3Dd v = new Vector3Dd(coord.x(), coord.y(), coord.z());
        ElementHierarchyState.activeState().coords.add(v);
        return v;
    }

    private static Vector3Dd monteCarloRadiosityInstallNormal(Vector3Dd normal) {
        Vector3Dd v = new Vector3Dd(normal.x(), normal.y(), normal.z());
        ElementHierarchyState.activeState().normals.add(v);
        return v;
    }

    private static Vector3Dd monteCarloRadiosityInstallTexCoord(Vector3Dd texCoord) {
        Vector3Dd t = new Vector3Dd(texCoord.x(), texCoord.y(), texCoord.z());
        ElementHierarchyState.activeState().texCoords.add(t);
        return t;
    }

    private static RpkVertex monteCarloRadiosityInstallVertex(Vector3Dd coord, Vector3Dd normal, Vector3Dd texCoord) {
        ArrayList<Patch> newPatchList = new ArrayList<>();
        RpkVertex v = new RpkVertex(coord, normal, texCoord, newPatchList);
        ElementHierarchyState.activeState().vertices.add(v);
        return v;
    }

    private static RpkVertex monteCarloRadiosityNewMidpointVertex(StochasticRadiosityElement elem, RpkVertex v1, RpkVertex v2) {
        Vector3Dd coord = new Vector3Dd();
        Vector3Dd norm = new Vector3Dd();
        Vector3Dd texCoord = new Vector3Dd();
        Vector3Dd p;
        Vector3Dd n;
        Vector3Dd t;

        coord = v1.point.add(v2.point).multiply(0.5);
        p = monteCarloRadiosityInstallCoordinate(coord);

        if ( v1.normal != null && v2.normal != null ) {
            norm = v1.normal.add(v2.normal).multiply(0.5);
            n = monteCarloRadiosityInstallNormal(norm);
        } else {
            n = elem.patch.getNormal();
        }

        if ( v1.textureCoordinates != null && v2.textureCoordinates != null ) {
            texCoord = v1.textureCoordinates.add(v2.textureCoordinates).multiply(0.5);
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
        RpkVertex from = elem.vertices[edgeNumber];
        RpkVertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];

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

    public static RpkVertex stochasticRadiosityElementEdgeMidpointVertex(StochasticRadiosityElement elem, int edgeNumber) {
        RpkVertex v = null;
        RpkVertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];
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
                            Logger.reportMessage(null, Logger.ERROR, "EdgeMidpointVertex", String.format("Invalid vertex index %d", index));
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
                            Logger.reportMessage(null, Logger.ERROR, "EdgeMidpointVertex", String.format("Invalid vertex index %d", index));
                    }
                    break;
                default:
                    Logger.reportMessage(null, Logger.FATAL_ERROR, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
            }
        }

        return v;
    }

    private static RpkVertex monteCarloRadiosityNewEdgeMidpointVertex(StochasticRadiosityElement elem, int edgeNumber) {
        RpkVertex v = StochasticRadiosityElement.stochasticRadiosityElementEdgeMidpointVertex(elem, edgeNumber);
        if ( v == null ) {
            // First time we split the edge, create the midpoint vertex
            RpkVertex from = elem.vertices[edgeNumber];
            RpkVertex to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices];
            v = monteCarloRadiosityNewMidpointVertex(elem, from, to);
        }
        return v;
    }

    private static Vector3Dd galerkinElementMidpoint(StochasticRadiosityElement elem) {
        elem.midPoint = new Vector3Dd(0.0, 0.0, 0.0);
        for ( int i = 0; i < elem.numberOfVertices; i++ ) {
            elem.midPoint = elem.midPoint.add(elem.vertices[i].point);
        }
        elem.midPoint = elem.midPoint.multiplyByInverse(elem.numberOfVertices, Numeric.EPSILON);

        return elem.midPoint;
    }

    /**
Only for surface elements
*/
    public static boolean stochasticRadiosityElementIsTextured(StochasticRadiosityElement elem) {
        if ( elem.isCluster() ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "stochasticRadiosityElementIsTextured", "this routine should not be called for cluster elements");
            return false;
        }
        RpkMaterial mat = elem.patch.getMaterial();
        return mat.getBsdf() != null
            && (mat.getBsdf().splitBsdfIsTextured() || PhongEmittanceDistributionFunction.edfIsTextured());
    }

    /**
Uses elem->Rd for surface elements
*/
    public static double stochasticRadiosityElementScalarReflectance(StochasticRadiosityElement elem) {
        double rd;

        if ( elem.isCluster() ) {
            return 1.0;
        }

        rd = (double)elem.Rd.maximumComponent();
        if ( rd < VSDK.EPSILON ) {
            // Avoid divisions by zero
            rd = Numeric.EPSILON;
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
        RpkRayHit hit = new RpkRayHit();
        hit.init(patch, patch.midPoint(), patch.getNormal(), patch.getMaterial());

        isTextured = StochasticRadiosityElement.stochasticRadiosityElementIsTextured(elem);
        numberOfSamples = isTextured ? 100 : 1;
        albedo = new ColorRgb(0.0, 0.0, 0.0);
        emittance = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRadiosityElement.stochasticRadiosityElementRange(elem, nbits, msb1, rMostSignificantBit2);

        for ( int i = 0; i < numberOfSamples; i++, n[0]++ ) {
            ColorRgb sample;
            long[] xi = Niederreiter.NextNiedInRange(n, +1, nbits[0], msb1[0], rMostSignificantBit2[0]);
            hit.setUv((double)xi[0] * Niederreiter.RECIP, (double)xi[1] * Niederreiter.RECIP);
            hit.setPoint(patch.uniformPoint(hit.getUv().x, hit.getUv().y));
            if ( patch.getMaterial().getBsdf() != null ) {
                sample = new ColorRgb(patch.getMaterial().getBsdf().splitBsdfScatteredPower(hit.shadingContext(), BsdfComponent.BRDF_DIFFUSE_COMPONENT));
                albedo = albedo.add(sample);
            }
            if ( patch.getMaterial().getEdf() != null ) {
                sample = new ColorRgb(patch.getMaterial().getEdf().phongEmittance(hit.shadingContext(), XxdfComponentFlag.DIFFUSE_COMPONENT));
                emittance = emittance.add(sample);
            }
        }
        elem.Rd = albedo.scaleInverse(numberOfSamples);
        elem.Ed = emittance.scaleInverse(numberOfSamples);
    }

    /**
Initial push operation for surface sub-elements
*/
    private static void monteCarloRadiosityInitSurfacePush(StochasticRadiosityElement parent, StochasticRadiosityElement child) {
        child.sourceRad = new ColorRgb(parent.sourceRad.getR(), parent.sourceRad.getG(), parent.sourceRad.getB());
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.radiance, child.radiance);
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.unShotRadiance, child.unShotRadiance);

        double[] parentImportance = new double[] {parent.importance};
        double[] childImportance = new double[] {child.importance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentImportance, childImportance);
        child.importance = childImportance[0];

        double[] parentSourceImportance = new double[] {parent.sourceImportance};
        double[] childSourceImportance = new double[] {child.sourceImportance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentSourceImportance, childSourceImportance);
        child.sourceImportance = childSourceImportance[0];

        double[] parentUnShotImportance = new double[] {parent.unShotImportance};
        double[] childUnShotImportance = new double[] {child.unShotImportance};
        StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentUnShotImportance, childUnShotImportance);
        child.unShotImportance = childUnShotImportance[0];

        child.rayIndex = parent.rayIndex;
        child.quality = parent.quality;
        child.samplingProbability = parent.samplingProbability * child.area / parent.area;

        child.Rd = new ColorRgb(parent.Rd.getR(), parent.Rd.getG(), parent.Rd.getB());
        child.Ed = new ColorRgb(parent.Ed.getR(), parent.Ed.getG(), parent.Ed.getB());
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
        RpkVertex v0,
        RpkVertex v1,
        RpkVertex v2,
        RpkVertex v3)
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

        elem.area = 0.25 * parent.area; // Regular elements, regular subdivision
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
        elem.importance = elem.unShotImportance = elem.receivedImportance = 0.0;
        monteCarloRadiosityInitSurfacePush(parent, elem);

        return elem;
    }

    /**
Create sub-elements: regular subdivision, see drawings above
*/
    private static StochasticRadiosityElement[] monteCarloRadiosityRegularSubdivideTriangle(StochasticRadiosityElement element, RendererConfiguration renderOptions) {
        RpkVertex v0 = element.vertices[0];
        RpkVertex v1 = element.vertices[1];
        RpkVertex v2 = element.vertices[2];
        RpkVertex m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
        RpkVertex m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
        RpkVertex m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);

        monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, m2, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 2, m2, m1, v2, null);
        monteCarloRadiosityCreateSurfaceSubElement(element, 3, m1, m2, m0, null);

        return castElementArray(element.regularSubElements);
    }

    private static StochasticRadiosityElement[] monteCarloRadiosityRegularSubdivideQuad(StochasticRadiosityElement element, RendererConfiguration renderOptions) {
        RpkVertex v0 = element.vertices[0];
        RpkVertex v1 = element.vertices[1];
        RpkVertex v2 = element.vertices[2];
        RpkVertex v3 = element.vertices[3];
        RpkVertex m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
        RpkVertex m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
        RpkVertex m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);
        RpkVertex m3 = monteCarloRadiosityNewEdgeMidpointVertex(element, 3);
        RpkVertex mm = monteCarloRadiosityNewMidpointVertex(element, m0, m2);

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
        RendererConfiguration renderOptions)
    {
        if ( element.regularSubElements != null ) {
            return castElementArray(element.regularSubElements);
        }

        if ( element.isCluster() ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
            return null;
        }

        if ( element.patch.getFace().hasJacobian() ) {
            Logger.reportMessage(null, Logger.WARNING, "galerkinElementRegularSubDivide", "irregular quadrilateral patches are not correctly handled (but you probably will not notice it)");
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
                Logger.reportMessage(null, Logger.FATAL_ERROR, "galerkinElementRegularSubDivide", "invalid element: not 3 or 4 vertices");
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
            childRadiance[0] = childRadiance[0].add(parentRadiance[0]);
        } else if ( regularChild(child) && child.basis == parent.basis ) {
            Basismcrad.filterColorDown(parentRadiance, child.basis.regularFilter[child.childNumber], childRadiance,
                child.basis.size);
        } else {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "stochasticRadiosityElementPushRadiance", "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
        }
    }

    public static void stochasticRadiosityElementPushImportance(double[] parentImportance, double[] childImportance) {
        childImportance[0] += parentImportance[0];
    }

    public static void stochasticRadiosityElementPullRadiance(
        StochasticRadiosityElement parent,
        StochasticRadiosityElement child,
        ColorRgb[] parentRad,
        ColorRgb[] childRad)
    {
        double areaFactor = child.area / parent.area;
        if ( parent.isCluster() || child.basis.size == 1 ) {
            parentRad[0] = parentRad[0].addScaled(areaFactor, childRad[0]);
        } else if ( regularChild(child) && child.basis == parent.basis ) {
            Basismcrad.filterColorUp(childRad, child.basis.regularFilter[child.childNumber],
                parentRad, child.basis.size, areaFactor);
        } else {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "stochasticRadiosityElementPullRadiance", "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
        }
    }

    public static void stochasticRadiosityElementPullImportance(StochasticRadiosityElement parent, StochasticRadiosityElement child, double[] parentImportance, double[] childImportance) {
        parentImportance[0] += child.area / parent.area * childImportance[0];
    }

    public static ColorRgb stochasticRadiosityElementColor(StochasticRadiosityElement element) {
        ColorRgb color = new ColorRgb();

        switch ( StochasticRelaxation.activeState().show ) {
            case SHOW_TOTAL_RADIANCE:
            case SHOW_INDIRECT_RADIANCE:
                color = ToneMap.radianceToRgb(
                    StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(element),
                    StochasticRelaxation.activeState().toneMapOptions);
                break;
            case SHOW_IMPORTANCE: {
                double gray;

                if ( element.importance > 1.0 ) {
                    gray = 1.0;
                } else {
                    gray = element.importance < 0.0 ? 0.0 : element.importance;
                }

                color = new ColorRgb(gray, gray, gray);
                break;
            }
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "stochasticRadiosityElementColor", String.format("Do not know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation.activeState().show.ordinal()));
        }

        return color;
    }

    private static ColorRgb vertexRadiance(RpkVertex v) {
        int count = 0;
        ColorRgb radiance = new ColorRgb();

        radiance = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; v.radianceData != null && i < v.radianceData.size(); i++ ) {
            Element element = v.radianceData.get(i);
            if ( element.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement elem = (StochasticRadiosityElement)element;
            if ( elem.regularSubElements == null ) {
                ColorRgb elementRadiosity = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
                radiance = radiance.add(elementRadiosity);
                count++;
            }
        }

        if ( count > 0 ) {
            radiance = radiance.scaleInverse(count);
        }

        return radiance;
    }

    /**
Same as above but for importance
*/
    private static double vertexImportance(RpkVertex v) {
        int count = 0;
        double imp = 0.0;

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
            imp /= (double)count;
        }

        return imp;
    }

    private static ColorRgb vertexColor(RpkVertex v) {
        switch ( StochasticRelaxation.activeState().show ) {
            case SHOW_TOTAL_RADIANCE:
            case SHOW_INDIRECT_RADIANCE:
                v.color = ToneMap.radianceToRgb(
                    vertexRadiance(v),
                    StochasticRelaxation.activeState().toneMapOptions);
                break;
            case SHOW_IMPORTANCE: {
                double gray = vertexImportance(v);
                if ( gray > 1.0 ) {
                    gray = 1.0;
                }
                if ( gray < 0.0 ) {
                    gray = 0.0;
                }
                v.color = new ColorRgb(gray, gray, gray);
                break;
            }
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "vertexColor", String.format("Do not know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation.activeState().show.ordinal()));
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
        RpkVertex[] m = new RpkVertex[4];
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
                    m[i].color = new ColorRgb(
                        (m[i].color.getR() + color.getR()) * 0.5,
                        (m[i].color.getG() + color.getG()) * 0.5,
                        (m[i].color.getB() + color.getB()) * 0.5);
                }
            }
        }
    }

    public static ColorRgb stochasticRadiosityElementDisplayRadiance(StochasticRadiosityElement elem) {
        ColorRgb radiance = new ColorRgb();
        radiance = elem.radiance[0].subtract(elem.sourceRad);

        if ( StochasticRelaxation.activeState().show != WhatToShow.SHOW_INDIRECT_RADIANCE ) {
            // sourceRad is self-emitted radiance when indirect-only is disabled.
            // Otherwise it represents direct illumination.
            radiance = radiance.add(elem.sourceRad);
            if ( StochasticRelaxation.activeState().indirectOnly != 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot != 0 ) {
                // Add self-emitted radiance
                radiance = radiance.add(elem.Ed);
            }
        }
        return radiance;
    }

    public static ColorRgb stochasticRadiosityElementDisplayRadianceAtPoint(
        StochasticRadiosityElement elem,
        double u,
        double v,
        RendererConfiguration renderOptions)
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
                        radiance = new ColorRgb(
                            rad[0].getR() + (double)u * (rad[1].getR() - rad[0].getR()) + (double)v * (rad[2].getR() - rad[0].getR()),
                            rad[0].getG() + (double)u * (rad[1].getG() - rad[0].getG()) + (double)v * (rad[2].getG() - rad[0].getG()),
                            rad[0].getB() + (double)u * (rad[1].getB() - rad[0].getB()) + (double)v * (rad[2].getB() - rad[0].getB()));
                        break;
                    case 4:
                        {
                            final double cc = (double)u * (double)v;
                            final double bb = (double)u - cc;
                            final double dd = (double)v - cc;
                            radiance = new ColorRgb(
                                rad[0].getR() + bb * (rad[1].getR() - rad[0].getR()) + cc * (rad[2].getR() - rad[0].getR()) + dd * (rad[3].getR() - rad[0].getR()),
                                rad[0].getG() + bb * (rad[1].getG() - rad[0].getG()) + cc * (rad[2].getG() - rad[0].getG()) + dd * (rad[3].getG() - rad[0].getG()),
                                rad[0].getB() + bb * (rad[1].getB() - rad[0].getB()) + cc * (rad[2].getB() - rad[0].getB()) + dd * (rad[3].getB() - rad[0].getB()));
                        }
                        break;
                    default:
                        Logger.reportMessage(null, Logger.FATAL_ERROR, "stochasticRadiosityElementDisplayRadianceAtPoint", "can only handle triangular or quadrilateral elements");
                }
            } else {
                // Flat shading
                radiance = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
            }
        } else {
            // Higher order approximations
            radiance = Basismcrad.colorAtUv(elem.basis, elem.radiance, u, v);
            if ( StochasticRelaxation.activeState().show == WhatToShow.SHOW_INDIRECT_RADIANCE ) {
                radiance = radiance.subtract(elem.sourceRad);
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

    public RpkGeometry getGeometry() {
        return geometry;
    }

    public void setGeometry(RpkGeometry inGeometry) {
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
