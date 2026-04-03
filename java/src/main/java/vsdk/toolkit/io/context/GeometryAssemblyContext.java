package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.Vertex;

public class GeometryAssemblyContext {
    public static final int MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

    public String currentVertexName;
    public int geometryStackHeadIndex;
    public ArrayList<Geometry>[] geometryStack;

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    public ArrayList<Vector3D> currentPointList;
    public ArrayList<Vector3D> currentNormalList;
    public ArrayList<Vertex> currentVertexList;
    public ArrayList<Patch> currentFaceList;
    public ArrayList<Geometry> currentGeometryList;
    public String currentObjectName;
    public boolean inSurface;
    public boolean inComplex;
    public boolean warpConeEnds;
    public ArrayList<Geometry> allGeometries;

    // Return model geometry output
    public ArrayList<Geometry> geometries;

    @SuppressWarnings("unchecked")
    public GeometryAssemblyContext() {
        currentVertexName = null;
        geometryStackHeadIndex = 0;
        geometryStack = (ArrayList<Geometry>[])new ArrayList[MAXIMUM_GEOMETRY_STACK_DEPTH];
        currentPointList = null;
        currentNormalList = null;
        currentVertexList = null;
        currentFaceList = null;
        currentGeometryList = null;
        currentObjectName = null;
        inSurface = false;
        inComplex = false;
        warpConeEnds = false;
        allGeometries = new ArrayList<>();
        geometries = null;
    }

    public void destroy() {
        currentObjectName = null;
        if (allGeometries != null) {
            allGeometries.clear();
            allGeometries = null;
        }
    }
}
