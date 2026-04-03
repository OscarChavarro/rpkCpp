package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.Vertex;

@FunctionalInterface
interface REFINE_ACTION {
    Link apply(
        Link link,
        StochasticRadiosityElement rcvtop,
        double[] ur,
        double[] vr,
        StochasticRadiosityElement srctop,
        double[] us,
        double[] vs,
        RenderOptions renderOptions);
}

@FunctionalInterface
interface ORACLE {
    REFINE_ACTION apply(Link link);
}

/**
Global parameters controlling hierarchical refinement
*/
public class ElementHierarchyState {
    public float epsilon; // Determines meshing accuracy
    public int do_h_meshing; // If doing hierarchical meshing
    public float minimumArea; // Minimum allowed element area
    public long nr_elements; // Number of elements
    public long nr_clusters; // Number of clusters
    public int tvertex_elimination; // If doing T-vertex elimination for rendering
    public HierarchyClusteringMode clustering; // Clustering mode, 0 => no clustering
    public ORACLE oracle; // Refinement oracle to be used
    public StochasticRadiosityElement topCluster; // Top cluster element of element hierarchy
    public ArrayList<Vector3D> coords;
    public ArrayList<Vector3D> normals; // Created during element subdivision
    public ArrayList<Vector3D> texCoords; // Created during element subdivision
    public ArrayList<Vertex> vertices;

    public ElementHierarchyState() {
        epsilon = 0.0f;
        do_h_meshing = 0;
        minimumArea = 0.0f;
        nr_elements = 0;
        nr_clusters = 0;
        tvertex_elimination = 0;
        clustering = null;
        oracle = null;
        topCluster = null;
        coords = null;
        normals = null;
        texCoords = null;
        vertices = null;
    }

    public static void setActiveState(ElementHierarchyState state) {
        activeStatePtr = state;
    }

    public static ElementHierarchyState activeState() {
        if ( activeStatePtr == null ) {
            Error.fatal(-1, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
        }
        return activeStatePtr;
    }

    private static ElementHierarchyState activeStatePtr = null;
}
