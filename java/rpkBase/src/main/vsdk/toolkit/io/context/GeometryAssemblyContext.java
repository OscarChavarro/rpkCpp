package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

public class GeometryAssemblyContext {
    public static final int MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

    public String currentVertexName;
    public int geometryStackHeadIndex;
    public ArrayList<RpkGeometry>[] geometryStack;

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    public ArrayList<Vector3Dd> currentPointList;
    public ArrayList<Vector3Dd> currentNormalList;
    public ArrayList<RpkVertex> currentVertexList;
    public ArrayList<Patch> currentFaceList;
    public ArrayList<RpkGeometry> currentGeometryList;
    public String currentObjectName;
    public boolean inSurface;
    public boolean inComplex;
    public ArrayList<RpkGeometry> allGeometries;

    // Return model geometry output
    public ArrayList<RpkGeometry> geometries;

    @SuppressWarnings("unchecked")
    public GeometryAssemblyContext() {
        currentVertexName = null;
        geometryStackHeadIndex = 0;
        geometryStack = (ArrayList<RpkGeometry>[])new ArrayList[MAXIMUM_GEOMETRY_STACK_DEPTH];
        currentPointList = null;
        currentNormalList = null;
        currentVertexList = null;
        currentFaceList = null;
        currentGeometryList = null;
        currentObjectName = null;
        inSurface = false;
        inComplex = false;
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
