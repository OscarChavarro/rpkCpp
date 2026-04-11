import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { GeometryAssemblyContext } from "../context/GeometryAssemblyContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { MeshSurfaceVisitor } from "../../numericalAnalysis/MeshSurfaceVisitor";
import { Compound } from "../../skin/Compound";
import { Geometry } from "../../skin/Geometry";
import { MaterialColorFlags } from "../../skin/MaterialColorFlags";
import { MeshSurface } from "../../skin/MeshSurface";
import { Patch } from "../../skin/Patch";
import { Vertex } from "../../skin/Vertex";
import { Material } from "../../material/Material";
import { MgfEntityControl } from "./MgfEntityControl";

/**
Hierarchical object names tracking.
*/
export class MgfObjectNameSupport {
  private constructor() {
  }

  private static disposeCurrentSurfaceLists(context: ParseRuntimeContext): void {
    if (context.currentPointList !== null) {
      context.currentPointList.length = 0;
      context.currentPointList = null;
    }
    if (context.currentNormalList !== null) {
      context.currentNormalList.length = 0;
      context.currentNormalList = null;
    }
    if (context.currentVertexList !== null) {
      context.currentVertexList.length = 0;
      context.currentVertexList = null;
    }
    if (context.currentFaceList !== null) {
      context.currentFaceList.length = 0;
      context.currentFaceList = null;
    }
  }

  private static pushCurrentGeometryList(context: ParseRuntimeContext): void {
    if (context.geometryStackHeadIndex >= GeometryAssemblyContext.MAXIMUM_GEOMETRY_STACK_DEPTH) {
      MgfEntityControl.doError(
        "Objects are nested too deep for this program. Recompile with larger GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf",
        context,
      );
      return;
    }
    context.geometryStack[context.geometryStackHeadIndex] = context.currentGeometryList;
    context.geometryBuildState.geometryStack[context.geometryStackHeadIndex] = context.currentGeometryList;
    context.geometryStackHeadIndex++;
    context.geometryBuildState.geometryStackHeadIndex = context.geometryStackHeadIndex;
    context.currentGeometryList = null;
    context.geometryBuildState.currentGeometryList = null;
  }

  private static popCurrentGeometryList(context: ParseRuntimeContext): void {
    if (context.geometryStackHeadIndex < 0) {
      MgfEntityControl.doError("Object stack underflow ... unbalanced 'o' contexts?", context);
      context.currentGeometryList = null;
      context.geometryBuildState.currentGeometryList = null;
      return;
    }
    context.geometryStackHeadIndex--;
    context.geometryBuildState.geometryStackHeadIndex = context.geometryStackHeadIndex;
    context.currentGeometryList = context.geometryStack[context.geometryStackHeadIndex];
    context.geometryBuildState.currentGeometryList = context.currentGeometryList;
  }

  public static mgfObjectNewSurface(context: ParseRuntimeContext): void {
    // Lists created here are transferred to MeshSurface.
    context.currentPointList = [];
    context.currentNormalList = [];
    context.currentVertexList = [];
    context.currentFaceList = [];
    context.inSurface = true;

    context.geometryBuildState.currentPointList = context.currentPointList;
    context.geometryBuildState.currentNormalList = context.currentNormalList;
    context.geometryBuildState.currentVertexList = context.currentVertexList;
    context.geometryBuildState.currentFaceList = context.currentFaceList;
    context.geometryBuildState.inSurface = true;
  }

  /**
  Handle object entity statement.
  */
  private static handleObject2Entity(ac: number, av: string[], context: ParseRuntimeContext): number {
    if (ac === 1) {
      // Just pop top object
      return context.objectHierarchyState.popName();
    }
    if (ac !== 2) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if (!TokenValidationContext.isName(av[1])) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    return context.objectHierarchyState.pushName(av[1]);
  }

  public static mgfObjectSurfaceDone(context: ParseRuntimeContext): void {
    if (context.currentGeometryList === null) {
      context.currentGeometryList = [];
      context.geometryBuildState.currentGeometryList = context.currentGeometryList;
    }

    if (context.readerContext !== null && context.readerContext.inputLine !== null) {
      const head = context.readerContext.inputLine;
      if (head.length >= 2 && head.charAt(0) === "o" && head.charAt(1) === " ") {
        let tail = head.substring(2);
        if (tail.length > 0) {
          if (context.currentObjectName !== null) {
            context.currentObjectName = null;
          }
          tail = tail.trim();
          context.currentObjectName = tail;
          context.geometryBuildState.currentObjectName = tail;
        }
      }
    }

    if (context.currentFaceList !== null && context.currentFaceList.length > 0) {
      const newGeometry = new MeshSurface(
        context.currentObjectName as unknown as string,
        context.currentMaterial as unknown as Material,
        context.currentPointList,
        context.currentNormalList,
        null, // texture coordinate list
        context.currentVertexList,
        context.currentFaceList,
        MaterialColorFlags.NO_COLORS,
      );
      MeshSurfaceVisitor.initializeFacesDefaults(newGeometry);
      context.currentGeometryList.push(newGeometry);
      (context.allGeometries as Geometry[]).push(newGeometry);
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
    }
    else {
      MgfObjectNameSupport.disposeCurrentSurfaceLists(context);
      context.geometryBuildState.currentPointList = null;
      context.geometryBuildState.currentNormalList = null;
      context.geometryBuildState.currentVertexList = null;
      context.geometryBuildState.currentFaceList = null;
    }
    context.inSurface = false;
    context.geometryBuildState.inSurface = false;
  }

  public static handleObjectEntity(argc: number, argv: string[], context: ParseRuntimeContext): number {
    if (argc > 1) {
      // Beginning of a new object
      if (context.inSurface) {
        MgfObjectNameSupport.mgfObjectSurfaceDone(context);
      }

      MgfObjectNameSupport.pushCurrentGeometryList(context);
      MgfObjectNameSupport.mgfObjectNewSurface(context);
    }
    else {
      // End of object definition
      let newGeometry: Geometry | null = null;

      if (context.inSurface) {
        MgfObjectNameSupport.mgfObjectSurfaceDone(context);
      }

      let listSize = 0;
      if (context.currentGeometryList !== null) {
        listSize += context.currentGeometryList.length;
      }

      if (listSize > 0) {
        newGeometry = new Compound(context.currentGeometryList);
      }

      MgfObjectNameSupport.popCurrentGeometryList(context);

      if (newGeometry !== null && context.currentGeometryList !== null) {
        context.currentGeometryList.push(newGeometry);
        context.geometries = context.currentGeometryList;
        context.geometryBuildState.geometries = context.geometries;
        (context.allGeometries as Geometry[]).push(newGeometry);
        MgfObjectNameSupport.mgfObjectNewSurface(context);
      }
    }

    return MgfObjectNameSupport.handleObject2Entity(argc, argv, context);
  }

  public static mgfObjectFreeMemory(context: ParseRuntimeContext): void {
    if (context !== null) {
      context.objectHierarchyState.clear();
    }
  }
}
