/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.Vertex;

public final class Hierarchy {
    private static final float DEFAULT_EH_EPSILON = 5e-4f;
    private static final float DEFAULT_EH_MINIMUM_AREA = 1e-6f;
    private static final boolean DEFAULT_EH_HIERARCHICAL_MESHING = true;
    private static final boolean DEFAULT_EH_T_VERTEX_ELIMINATION = true;
    private static final HierarchyClusteringMode DEFAULT_EH_CLUSTERING = HierarchyClusteringMode.ORIENTED_CLUSTERING;
    private static final REFINE_ACTION DONT_REFINE_ACTION = Hierarchy::dontRefineCallBack;

    private Hierarchy() {
    }

    public static void elementHierarchyDefaults() {
        ElementHierarchyState.activeState().epsilon = DEFAULT_EH_EPSILON;
        ElementHierarchyState.activeState().minimumArea = DEFAULT_EH_MINIMUM_AREA;
        ElementHierarchyState.activeState().do_h_meshing = DEFAULT_EH_HIERARCHICAL_MESHING ? 1 : 0;
        ElementHierarchyState.activeState().clustering = DEFAULT_EH_CLUSTERING;
        ElementHierarchyState.activeState().tvertex_elimination = DEFAULT_EH_T_VERTEX_ELIMINATION ? 1 : 0;
        ElementHierarchyState.activeState().oracle = Hierarchy::powerOracle;
        ElementHierarchyState.activeState().nr_elements = 0;
        ElementHierarchyState.activeState().nr_clusters = 0;
    }

    public static void elementHierarchyInit(Geometry clusteredWorldGeometry) {
        // These lists hold vertices created during hierarchical refinement
        ElementHierarchyState.activeState().coords = new ArrayList<>();
        ElementHierarchyState.activeState().normals = new ArrayList<>();
        ElementHierarchyState.activeState().texCoords = new ArrayList<>();
        ElementHierarchyState.activeState().vertices = new ArrayList<>();
        ElementHierarchyState.activeState().topCluster =
            StochasticRadiosityElement.stochasticRadiosityElementCreateFromGeometry(clusteredWorldGeometry);
    }

    public static void elementHierarchyTerminate(ArrayList<Patch> scenePatches) {
        // Destroy clusters
        StochasticRadiosityElement.stochasticRadiosityElementDestroyClusterHierarchy(ElementHierarchyState.activeState().topCluster);
        ElementHierarchyState.activeState().topCluster = null;

        // Destroy surface elements
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            // Need to be destroyed before destroying the automatically created vertices
            StochasticRadiosityElement.stochasticRadiosityElementDestroy(McradP.topLevelStochasticRadiosityElement(patch));
            patch.radianceData = null; // Prevents destroying a 2nd time later
        }

        // Delete vertices
        ArrayList<Vertex> vertices = ElementHierarchyState.activeState().vertices;
        if ( vertices != null ) {
            for ( int i = 0; i < vertices.size(); i++ ) {
                vertices.set(i, null);
            }
            vertices.clear();
        }
        ElementHierarchyState.activeState().vertices = null;

        // Delete positions
        if ( ElementHierarchyState.activeState().coords != null ) {
            ElementHierarchyState.activeState().coords.clear();
            ElementHierarchyState.activeState().coords = null;
        }

        // Delete normals
        if ( ElementHierarchyState.activeState().normals != null ) {
            ElementHierarchyState.activeState().normals.clear();
            ElementHierarchyState.activeState().normals = null;
        }

        // Delete texture coordinates
        if ( ElementHierarchyState.activeState().texCoords != null ) {
            ElementHierarchyState.activeState().texCoords.clear();
            ElementHierarchyState.activeState().texCoords = null;
        }
    }

    /**
Refinement action 1: do nothing (link is accurate enough)
Special refinement action used to indicate that a link is admissible
*/
    private static Link dontRefineCallBack(
        Link link,
        StochasticRadiosityElement rcvtop,
        double[] ur,
        double[] vr,
        StochasticRadiosityElement srctop,
        double[] us,
        double[] vs,
        RenderOptions renderOptions)
    {
        // Doesn't do anything
        return link;
    }

    /**
Refinement action 2: subdivide the receiver using regular quadtree subdivision
*/
    private static Link subdivideReceiverCallBack(
        Link link,
        StochasticRadiosityElement rcvtop,
        double[] ur,
        double[] vr,
        StochasticRadiosityElement srctop,
        double[] us,
        double[] vs,
        RenderOptions renderOptions)
    {
        StochasticRadiosityElement rcv = link.rcv;
        if ( rcv.isCluster() ) {
            rcv = (StochasticRadiosityElement)rcv.childContainingElement(rcvtop);
        } else {
            if ( rcv.regularSubElements == null ) {
                StochasticRadiosityElement.stochasticRadiosityElementRegularSubdivideElement(rcv, renderOptions);
            }
            rcv = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(rcv, ur, vr);
        }
        link.rcv = rcv;
        return link;
    }

    /**
Refinement action 3: subdivide the source using regular quadtree subdivision
*/
    private static Link subdivideSourceCallBack(
        Link link,
        StochasticRadiosityElement rcvtop,
        double[] ur,
        double[] vr,
        StochasticRadiosityElement srcTop,
        double[] us,
        double[] vs,
        RenderOptions renderOptions)
    {
        StochasticRadiosityElement src = link.src;
        if ( src.isCluster() ) {
            src = (StochasticRadiosityElement)src.childContainingElement(srcTop);
        } else {
            if ( src.regularSubElements == null ) {
                StochasticRadiosityElement.stochasticRadiosityElementRegularSubdivideElement(src, renderOptions);
            }
            src = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(src, us, vs);
        }
        link.src = src;
        return link;
    }

    static boolean selfLink(Link link) {
        return (link.rcv == link.src);
    }

    /**
Cheap form factor estimate to drive hierarchical refinement
as in Hanrahan SIGGRAPH'91
*/
    static float formFactorEstimate(StochasticRadiosityElement rcv, StochasticRadiosityElement src) {
        Vector3D D = new Vector3D();
        D.subtraction(src.midPoint, rcv.midPoint);

        double d = D.norm();
        double f = src.area / (Math.PI * d * d + src.area);
        double f2 = 2.0 * f;
        double c1 = rcv.isCluster() ? 1.0 : Math.abs(D.dotProduct(rcv.patch.normal)) / d;
        if ( c1 < f2 ) {
            c1 = f2;
        }
        double c2 = src.isCluster() ? 1.0 : Math.abs(D.dotProduct(src.patch.normal)) / d;
        if ( c2 < f2 ) {
            c2 = f2;
        }
        return (float)(f * c1 * c2);
    }

    static boolean lowPowerLink(Link link, Statistics statistics) {
        StochasticRadiosityElement rcv = link.rcv;
        StochasticRadiosityElement src = link.src;
        ColorRgb rhoSrcRad = new ColorRgb();
        float ff = formFactorEstimate(rcv, src);
        float threshold;
        float propagatedPower;

        // Compute receiver reflectance times source radiosity
        rhoSrcRad.scaledCopy((float)Math.PI, src.radiance[0]);
        if ( !rcv.isCluster() ) {
            ColorRgb rd = McradP.topLevelStochasticRadiosityElement(rcv.patch).Rd;
            rhoSrcRad.selfScalarProduct(rd);
        }

        threshold = ElementHierarchyState.activeState().epsilon * statistics.radiance.maxSelfEmittedPower.maximumComponent();
        propagatedPower = rcv.area * ff * rhoSrcRad.maximumComponent();
        if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
            propagatedPower *= rcv.importance;
            if ( !rcv.isCluster() ) {
                propagatedPower *= StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(rcv);
            }
        }

        return (propagatedPower < threshold);
    }

    static REFINE_ACTION subDivideLargest(Link link) {
        StochasticRadiosityElement rcv = link.rcv;
        StochasticRadiosityElement src = link.src;
        if ( rcv.area < ElementHierarchyState.activeState().minimumArea && src.area < ElementHierarchyState.activeState().minimumArea ) {
            return DONT_REFINE_ACTION;
        } else {
            return (rcv.area > src.area) ? Hierarchy::subdivideReceiverCallBack : Hierarchy::subdivideSourceCallBack;
        }
    }

    /**
Well known power-based refinement oracle ([HANR1992] Hanrahan'91, with importance
a la [SMIT1992] Smits'92 when importance-driven sampling is enabled)
*/
    public static REFINE_ACTION powerOracle(Link link) {
        if ( selfLink(link) ) {
            return Hierarchy::subdivideReceiverCallBack;
        } else if ( lowPowerLink(link, Statistics.instance()) ) {
            return DONT_REFINE_ACTION;
        } else {
            return subDivideLargest(link);
        }
    }

    /**
Constructs a toplevel link for given toplevel surface elements
rcvTop and srcTop: the result is a link between the toplevel
cluster containing the whole scene and itself if clustering is
enabled. If clustering is not enabled, a link between the
given toplevel surface elements is returned
*/
    public static Link topLink(StochasticRadiosityElement rcvTop, StochasticRadiosityElement srcTop) {
        StochasticRadiosityElement rcv;
        StochasticRadiosityElement src;
        Link link = new Link();

        if ( ElementHierarchyState.activeState().do_h_meshing != 0
          && ElementHierarchyState.activeState().clustering != HierarchyClusteringMode.NO_CLUSTERING ) {
            src = rcv = ElementHierarchyState.activeState().topCluster;
        } else {
            src = srcTop;
            rcv = rcvTop;
        }

        link.rcv = rcv;
        link.src = src;

        return link;
    }

    /**
Refines a toplevel link (constructed with TopLink() above). The
returned Link structure contains pointers the admissible
elements and corresponding point coordinates for light transport.
rcvTop and srcTop are toplevel surface elements containing the
endpoint and origin respectively of a line along which light is to
be transported. (ur,vr) and (us,vs) are the uniform parameters of
the endpoint and origin on the toplevel surface elements on input.
They will be replaced by the point parameters on the admissible elements
after refinement
(ur,vr) are the coordinates of the point on the receiver patch,
(us,vs) coordinates of the point on the source patch
*/
    public static Link hierarchyRefine(
        Link link,
        StochasticRadiosityElement rcvTop,
        double[] ur,
        double[] vr,
        StochasticRadiosityElement srcTop,
        double[] us,
        double[] vs,
        ORACLE evaluateLink,
        RenderOptions renderOptions)
    {
        if ( ElementHierarchyState.activeState().do_h_meshing == 0 ) {
            link.rcv = rcvTop;
            link.src = srcTop;
        } else {
            REFINE_ACTION action;
            while ( (action = evaluateLink.apply(link)) != DONT_REFINE_ACTION ) {
                link = action.apply(link, rcvTop, ur, vr, srcTop, us, vs, renderOptions);
            }
        }
        return link;
    }
}
