package vsdk.toolkit.io.mgf;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.context.GeometryAssemblyContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.numericalAnalysis.MeshSurfaceVisitor;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.material.MaterialColorFlags;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.Vertex;

/**
Hierarchical object names tracking
*/
public class MgfObjectNameSupport {
    private static void disposeCurrentSurfaceLists(ParseRuntimeContext context) {
        if (context.currentPointList != null) {
            context.currentPointList.clear();
            context.currentPointList = null;
        }
        if (context.currentNormalList != null) {
            context.currentNormalList.clear();
            context.currentNormalList = null;
        }
        if (context.currentVertexList != null) {
            context.currentVertexList.clear();
            context.currentVertexList = null;
        }
        if (context.currentFaceList != null) {
            context.currentFaceList.clear();
            context.currentFaceList = null;
        }
    }

    private static void pushCurrentGeometryList(ParseRuntimeContext context) {
        if (context.geometryStackHeadIndex >= GeometryAssemblyContext.MAXIMUM_GEOMETRY_STACK_DEPTH) {
            MgfEntityControl.doError("Objects are nested too deep for this program. Recompile with larger GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
            return;
        } else {
            context.geometryStack[context.geometryStackHeadIndex] = context.currentGeometryList;
            context.geometryBuildState.geometryStack[context.geometryStackHeadIndex] = context.currentGeometryList;
            context.geometryStackHeadIndex++;
            context.geometryBuildState.geometryStackHeadIndex = context.geometryStackHeadIndex;
            context.currentGeometryList = null;
            context.geometryBuildState.currentGeometryList = null;
        }
    }

    private static void popCurrentGeometryList(ParseRuntimeContext context) {
        if (context.geometryStackHeadIndex < 0) {
            MgfEntityControl.doError("Object stack underflow ... unbalanced 'o' contexts?", context);
            context.currentGeometryList = null;
            context.geometryBuildState.currentGeometryList = null;
            return;
        } else {
            context.geometryStackHeadIndex--;
            context.geometryBuildState.geometryStackHeadIndex = context.geometryStackHeadIndex;
            context.currentGeometryList = context.geometryStack[context.geometryStackHeadIndex];
            context.geometryBuildState.currentGeometryList = context.currentGeometryList;
        }
    }

    public static void mgfObjectNewSurface(ParseRuntimeContext context) {
        // Note: lists created here will be transferred to new MeshSurface,
        // should not be deleted from MgfParseSession
        context.currentPointList = new ArrayList<Vector3D>();
        context.currentNormalList = new ArrayList<Vector3D>();
        context.currentVertexList = new ArrayList<Vertex>();
        context.currentFaceList = new ArrayList<Patch>();
        context.inSurface = true;

        context.geometryBuildState.currentPointList = context.currentPointList;
        context.geometryBuildState.currentNormalList = context.currentNormalList;
        context.geometryBuildState.currentVertexList = context.currentVertexList;
        context.geometryBuildState.currentFaceList = context.currentFaceList;
        context.geometryBuildState.inSurface = true;
    }

    /**
    Handle an object entity statement
    */
    private static int handleObject2Entity(int ac, String[] av, ParseRuntimeContext context) {
        if (ac == 1) {
            // Just pop top object
            return context.objectHierarchyState.popName();
        }
        if (ac != 2) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isName(av[1])) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        return context.objectHierarchyState.pushName(av[1]);
    }

    public static void mgfObjectSurfaceDone(ParseRuntimeContext context) {
        if (context.currentGeometryList == null) {
            context.currentGeometryList = new ArrayList<Geometry>();
            context.geometryBuildState.currentGeometryList = context.currentGeometryList;
        }

        if (context.readerContext != null && context.readerContext.inputLine != null) {
            String head = context.readerContext.inputLine;
            if (head.length() >= 2 && head.charAt(0) == 'o' && head.charAt(1) == ' ') {
                String tail = head.substring(2);
                if (!tail.isEmpty()) {
                    if (context.currentObjectName != null) {
                        context.currentObjectName = null;
                    }
                    tail = tail.trim();
                    context.currentObjectName = tail;
                    context.geometryBuildState.currentObjectName = tail;
                }
            }
        }

        if (context.currentFaceList != null && context.currentFaceList.size() > 0) {
            Geometry newGeometry = new MeshSurface(
                context.currentObjectName,
                context.currentMaterial,
                context.currentPointList,
                context.currentNormalList,
                null, // null texture coordinate list
                context.currentVertexList,
                context.currentFaceList,
                MaterialColorFlags.NO_COLORS);
            MeshSurfaceVisitor.initializeFacesDefaults((MeshSurface)newGeometry);
            context.currentGeometryList.add(newGeometry);
            context.allGeometries.add(newGeometry);
            context.geometryBuildState.allGeometries = context.allGeometries;
            context.currentObjectName = null;
            context.currentPointList = null;
            context.currentNormalList = null;
            context.currentVertexList = null;
            context.currentFaceList = null;

            context.geometryBuildState.currentObjectName = null;
            context.geometryBuildState.currentPointList = null;
            context.geometryBuildState.currentNormalList = null;
            context.geometryBuildState.currentVertexList = null;
            context.geometryBuildState.currentFaceList = null;
        } else {
            MgfObjectNameSupport.disposeCurrentSurfaceLists(context);
            context.geometryBuildState.currentPointList = null;
            context.geometryBuildState.currentNormalList = null;
            context.geometryBuildState.currentVertexList = null;
            context.geometryBuildState.currentFaceList = null;
        }
        context.inSurface = false;
        context.geometryBuildState.inSurface = false;
    }

    public static int handleObjectEntity(int argc, String[] argv, ParseRuntimeContext context) {
        if (argc > 1) {
            // Beginning of a new object
            if (context.inSurface) {
                MgfObjectNameSupport.mgfObjectSurfaceDone(context);
            }

            MgfObjectNameSupport.pushCurrentGeometryList(context);

            MgfObjectNameSupport.mgfObjectNewSurface(context);
        } else {
            // End of object definition
            Geometry newGeometry = null;

            if (context.inSurface) {
                MgfObjectNameSupport.mgfObjectSurfaceDone(context);
            }

            long listSize = 0;
            if (context.currentGeometryList != null) {
                listSize += context.currentGeometryList.size();
            }

            if (listSize > 0) {
                newGeometry = new Compound(context.currentGeometryList);
            }

            MgfObjectNameSupport.popCurrentGeometryList(context);

            if (newGeometry != null && context.currentGeometryList != null) {
                context.currentGeometryList.add(newGeometry);
                context.geometries = context.currentGeometryList;
                context.geometryBuildState.geometries = context.geometries;
                context.allGeometries.add(newGeometry);
                MgfObjectNameSupport.mgfObjectNewSurface(context);
            }
        }

        return MgfObjectNameSupport.handleObject2Entity(argc, argv, context);
    }

    public static void mgfObjectFreeMemory(ParseRuntimeContext context) {
        if (context != null) {
            context.objectHierarchyState.clear();
        }
    }
}
