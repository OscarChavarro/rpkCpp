package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.scene.RadianceElementTable;
import java.util.ArrayList;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

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
        RendererConfiguration renderOptions);
}

@FunctionalInterface
interface ORACLE {
    REFINE_ACTION apply(Link link);
}

/**
Global parameters controlling hierarchical refinement
*/
public class ElementHierarchyState {
    public double epsilon; // Determines meshing accuracy
    public int do_h_meshing; // If doing hierarchical meshing
    public double minimumArea; // Minimum allowed element area
    public long nr_elements; // Number of elements
    public long nr_clusters; // Number of clusters
    public int tvertex_elimination; // If doing T-vertex elimination for rendering
    public HierarchyClusteringMode clustering; // Clustering mode, 0 => no clustering
    public ORACLE oracle; // Refinement oracle to be used
    public StochasticRadiosityElement topCluster; // Top cluster element of element hierarchy
    public ArrayList<Vector3Dd> coords;
    public ArrayList<Vector3Dd> normals; // Created during element subdivision
    public ArrayList<Vector3Dd> texCoords; // Created during element subdivision
    public ArrayList<RpkVertex> vertices;

    public ElementHierarchyState() {
        epsilon = 0.0;
        do_h_meshing = 0;
        minimumArea = 0.0;
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
        radianceElements = null;
    }

    public static void setActiveState(ElementHierarchyState state) {
        activeStatePtr = state;
    }

    public static ElementHierarchyState activeState() {
        if ( activeStatePtr == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
        }
        return activeStatePtr;
    }

    public RadianceElementTable getRadianceElements() {
        return radianceElements;
    }

    public void setRadianceElements(RadianceElementTable inRadianceElements) {
        radianceElements = inRadianceElements;
    }

    // The scene's table of top-level elements, bound when the method is initialized;
    // null while the method has not been initialized (nothing to terminate then)
    private RadianceElementTable radianceElements;
    private static ElementHierarchyState activeStatePtr = null;
}
