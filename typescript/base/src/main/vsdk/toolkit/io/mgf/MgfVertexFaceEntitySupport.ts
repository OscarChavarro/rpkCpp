import { Logger } from "../../common/logging/Logger";
import { LookUpEntity } from "../../common/dataStructures/LookUpEntity";
import { CoordinateAxis } from "../../common/linealAlgebra/CoordinateAxis";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { TransformStackContext } from "../context/TransformStackContext";
import { VertexContext } from "../context/VertexContext";
import { Material } from "../../material/Material";
import { Patch } from "../../environment/geometry/elements/Patch";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { MgfConeEntityTessellator } from "./MgfConeEntityTessellator";
import { MgfCylinderEntityExpander } from "./MgfCylinderEntityExpander";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfFaceWithHolesEntityExpander } from "./MgfFaceWithHolesEntityExpander";
import { MgfMaterialEntitySupport } from "./MgfMaterialEntitySupport";
import { MgfObjectNameSupport } from "./MgfObjectNameSupport";
import { MgfPrismEntityTessellator } from "./MgfPrismEntityTessellator";
import { MgfRingEntityTessellator } from "./MgfRingEntityTessellator";
import { MgfSphereEntityExpander } from "./MgfSphereEntityExpander";
import { MgfTorusEntityExpander } from "./MgfTorusEntityExpander";
import { MgfTransformationSupport } from "./MgfTransformationSupport";

export class MgfVertexFaceEntitySupport {
  private static readonly MAXIMUM_FACE_VERTICES = 100;

  private constructor() {
  }

  private static transformXid(xf: TransformStackContext | null): number {
    return xf === null ? 0 : xf.xid;
  }

  /**
  The mgf parser already contains some good routines for discrete spheres /
  cone / cylinder / torus into polygons. In the official release of the parser
  library, these routines are internal. Here we expose them to avoid
  duplicating the code.
  */
  private static doDiscreteConic(argc: number, argv: string[], context: ParseRuntimeContext): number {
    const en = MgfEntityControl.mgfEntity(argv[0], context);

    switch (en) {
      case EntityTypeContext.SPHERE:
        return MgfSphereEntityExpander.handleEntity(argc, argv, context);
      case EntityTypeContext.TORUS:
        return MgfTorusEntityExpander.handleEntity(argc, argv, context);
      case EntityTypeContext.CYLINDER:
        return MgfCylinderEntityExpander.handleEntity(argc, argv, context);
      case EntityTypeContext.RING:
        return MgfRingEntityTessellator.handleEntity(argc, argv, context);
      case EntityTypeContext.CONE:
        return MgfConeEntityTessellator.handleEntity(argc, argv, context);
      case EntityTypeContext.PRISM:
        return MgfPrismEntityTessellator.handleEntity(argc, argv, context);
      default:
        Logger.fatal(4, "mgf.c: doDiscreteConic", "Unsupported geometry entity number %d", en);
        return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
    }
  }

  private static installPoint(x: number, y: number, z: number, context: ParseRuntimeContext): Vector3D {
    const coord = new Vector3D(x, y, z);
    if (context.currentPointList !== null) {
      context.currentPointList.push(coord);
    }
    return coord;
  }

  private static installNormal(x: number, y: number, z: number, context: ParseRuntimeContext): Vector3D {
    const norm = new Vector3D(x, y, z);
    if (context.currentNormalList !== null) {
      context.currentNormalList.push(norm);
    }
    return norm;
  }

  private static installVertex(coord: Vector3D, norm: Vector3D | null, context: ParseRuntimeContext): Vertex {
    const newPatchList: Patch[] = [];
    const vertex = new Vertex(coord, norm, null, newPatchList);
    if (context.currentVertexList !== null) {
      context.currentVertexList.push(vertex);
    }
    return vertex;
  }

  private static getVertex(name: string, context: ParseRuntimeContext): Vertex | null {
    const vp = MgfVertexFaceEntitySupport.getNamedVertex(name, context);
    if (vp === null) {
      return null;
    }

    let theVertex = vp.vertex;
    if (
      theVertex === null
      || vp.clock >= 1
      || vp.xid !== MgfVertexFaceEntitySupport.transformXid(context.transformContext)
      || vp.n.isNull(Numeric.EPSILON)
    ) {
      // New vertex, updated vertex, different transform, or no normal
      const vert = new Vector3Dd();
      const norm = new Vector3Dd();
      let theNormal: Vector3D | null;

      MgfTransformationSupport.mgfTransformPoint(vert, vp.p, context);
      const thePoint = MgfVertexFaceEntitySupport.installPoint(vert.x, vert.y, vert.z, context);
      if (vp.n.isNull(Numeric.EPSILON)) {
        theNormal = null;
      }
      else {
        MgfTransformationSupport.mgfTransformVector(norm, vp.n, context);
        theNormal = MgfVertexFaceEntitySupport.installNormal(norm.x, norm.y, norm.z, context);
      }
      theVertex = MgfVertexFaceEntitySupport.installVertex(thePoint, theNormal, context);
      vp.vertex = theVertex;
      vp.xid = MgfVertexFaceEntitySupport.transformXid(context.transformContext);
    }
    vp.clock = 0;

    return theVertex;
  }

  /**
  Create a vertex with reversed normal for back faces of two-sided surfaces.
  */
  private static getBackFaceVertex(v: Vertex, context: ParseRuntimeContext): Vertex {
    let back = v.back;

    if (back === null) {
      const point = v.point;
      let normal = v.normal;
      if (normal !== null) {
        normal = MgfVertexFaceEntitySupport.installNormal(-normal.x, -normal.y, -normal.z, context);
      }

      back = MgfVertexFaceEntitySupport.installVertex(point, normal, context);
      v.back = back;
      back.back = v;
    }

    return back;
  }

  private static newFace(v1: Vertex | null, v2: Vertex | null, v3: Vertex | null, v4: Vertex | null, context: ParseRuntimeContext): Patch | null {
    let theFace: Patch;
    const numberOfVertices = v4 !== null ? 4 : 3;

    if (v1 === null || v2 === null || v3 === null) {
      return null;
    }

    if (context.transformContext !== null && context.transformContext.rev !== 0) {
      theFace = new Patch(numberOfVertices, v3, v2, v1, v4);
    }
    else {
      theFace = new Patch(numberOfVertices, v1, v2, v3, v4);
    }

    // If radiance computations are active, create patch data.
    if (theFace.material !== null && context.radianceMethod !== null) {
      context.radianceMethod.createPatchData(theFace);
    }

    if (context.currentFaceList !== null) {
      context.currentFaceList.push(theFace);
    }

    return theFace;
  }

  /**
  Computes the normal to the patch plane.
  */
  private static faceNormal(numberOfVertices: number, v: Vertex[], normal: Vector3D): Vector3D | null {
    const cur = new Vector3D();
    const n = new Vector3D();

    n.set(0, 0, 0);
    cur.subtraction(v[numberOfVertices - 1].point, v[0].point);
    for (let i = 0; i < numberOfVertices; i++) {
      const prev = new Vector3D(cur.x, cur.y, cur.z);
      cur.subtraction(v[i].point, v[0].point);
      n.x += (prev.y - cur.y) * (prev.z + cur.z);
      n.y += (prev.z - cur.z) * (prev.x + cur.x);
      n.z += (prev.x - cur.x) * (prev.y + cur.y);
    }
    const localNorm = n.norm();

    if (localNorm < Numeric.EPSILON) {
      // Degenerate normal -> degenerate polygon
      return null;
    }
    n.inverseScaledCopy(localNorm, n, Numeric.EPSILON_FLOAT);
    normal.copy(n);

    return normal;
  }

  /**
  Given a vector p in 3D space and an index i (X/Y/Z), project on YZ/XZ/XY.
  */
  private static vectorProject(r: Vector2D, p: Vector3D, i: CoordinateAxis): void {
    switch (i) {
      case CoordinateAxis.X:
        r.x = p.y;
        r.y = p.z;
        break;
      case CoordinateAxis.Y:
        r.x = p.x;
        r.y = p.z;
        break;
      case CoordinateAxis.Z:
        r.x = p.x;
        r.y = p.y;
        break;
      default:
        break;
    }
  }

  /**
  Test whether polygon is convex by projected edge-cross sign consistency.
  */
  private static faceIsConvex(numberOfVertices: number, v: Vertex[], normal: Vector3D): boolean {
    const v2d = new Array<Vector2D>(MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES + 1);
    for (let i = 0; i < v2d.length; i++) {
      v2d[i] = new Vector2D();
    }
    const p = new Vector2D();
    const c = new Vector2D();

    const index = normal.dominantCoordinate();
    for (let i = 0; i < numberOfVertices; i++) {
      MgfVertexFaceEntitySupport.vectorProject(v2d[i], v[i].point, index);
    }

    p.x = v2d[3].x - v2d[2].x;
    p.y = v2d[3].y - v2d[2].y;
    c.x = v2d[0].x - v2d[3].x;
    c.y = v2d[0].y - v2d[3].y;
    const sign = (p.x * c.y > c.x * p.y) ? 1 : -1;

    for (let i = 1; i < numberOfVertices; i++) {
      p.x = c.x;
      p.y = c.y;
      c.x = v2d[i].x - v2d[i - 1].x;
      c.y = v2d[i].y - v2d[i - 1].y;
      if (((p.x * c.y > c.x * p.y) ? 1 : -1) !== sign) {
        return false;
      }
    }

    return true;
  }

  /**
  Returns true if 2D point p is inside triangle p1-p2-p3.
  */
  private static pointInsideTriangle2D(p: Vector2D, p1: Vector2D, p2: Vector2D, p3: Vector2D): boolean {
    // Graphics Gems I, Didier Badouel
    const u0 = p.x - p1.x;
    const v0 = p.y - p1.y;
    const u1 = p2.x - p1.x;
    const v1 = p2.y - p1.y;
    const u2 = p3.x - p1.x;
    const v2 = p3.y - p1.y;

    let a = 10.0;
    let b = 10.0; // Large enough so result would be false
    if (globalThis.Math.abs(u1) < Numeric.EPSILON) {
      if (globalThis.Math.abs(u2) > Numeric.EPSILON && globalThis.Math.abs(v1) > Numeric.EPSILON) {
        b = u0 / u2;
        if (b < Numeric.EPSILON || b > 1.0 - Numeric.EPSILON) {
          return false;
        }
        else {
          a = (v0 - b * v2) / v1;
        }
      }
    }
    else {
      b = v2 * u1 - u2 * v1;
      if (globalThis.Math.abs(b) > Numeric.EPSILON) {
        b = (v0 * u1 - u0 * v1) / b;
        if (b < Numeric.EPSILON || b > 1.0 - Numeric.EPSILON) {
          return false;
        }
        else {
          a = (u0 - b * u2) / u1;
        }
      }
    }

    return (a >= Numeric.EPSILON && a <= 1.0 - Numeric.EPSILON && (a + b) <= 1.0 - Numeric.EPSILON);
  }

  /**
  Returns true if 2D segments p1-p2 and p3-p4 intersect.
  */
  private static segmentsIntersect2D(p1: Vector2D, p2: Vector2D, p3: Vector2D, p4: Vector2D): boolean {
    let a: number;
    let b: number;
    let c: number;
    let coLinear = false;

    // Graphics Gems II, Mukesh Prasad
    let du = globalThis.Math.abs(p2.x - p1.x);
    let dv = globalThis.Math.abs(p2.y - p1.y);
    if (du > Numeric.EPSILON || dv > Numeric.EPSILON) {
      if (dv > du) {
        a = 1.0;
        b = -(p2.x - p1.x) / (p2.y - p1.y);
        c = -(p1.x + b * p1.y);
      }
      else {
        a = -(p2.y - p1.y) / (p2.x - p1.x);
        b = 1.0;
        c = -(a * p1.x + p1.y);
      }

      const r3 = a * p3.x + b * p3.y + c;
      const r4 = a * p4.x + b * p4.y + c;

      if (globalThis.Math.abs(r3) < Numeric.EPSILON && globalThis.Math.abs(r4) < Numeric.EPSILON) {
        coLinear = true;
      }
      else if ((r3 > -Numeric.EPSILON && r4 > -Numeric.EPSILON) || (r3 < Numeric.EPSILON && r4 < Numeric.EPSILON)) {
        return false;
      }
    }

    if (coLinear === false) {
      du = globalThis.Math.abs(p4.x - p3.x);
      dv = globalThis.Math.abs(p4.y - p3.y);
      if (du > Numeric.EPSILON || dv > Numeric.EPSILON) {
        if (dv > du) {
          a = 1.0;
          b = -(p4.x - p3.x) / (p4.y - p3.y);
          c = -(p3.x + b * p3.y);
        }
        else {
          a = -(p4.y - p3.y) / (p4.x - p3.x);
          b = 1.0;
          c = -(a * p3.x + p3.y);
        }

        const r1 = a * p1.x + b * p1.y + c;
        const r2 = a * p2.x + b * p2.y + c;

        if (globalThis.Math.abs(r1) < Numeric.EPSILON && globalThis.Math.abs(r2) < Numeric.EPSILON) {
          coLinear = true;
        }
        else if ((r1 > -Numeric.EPSILON && r2 > -Numeric.EPSILON) || (r1 < Numeric.EPSILON && r2 < Numeric.EPSILON)) {
          return false;
        }
      }
    }

    if (coLinear === false) {
      return true;
    }

    // Co-linear segments never intersect: treat as slightly apart.
    return false;
  }

  /**
  Handles concave faces and faces with >4 vertices.
  */
  private static doComplexFace(
    n: number,
    v: Vertex[],
    normal: Vector3D,
    backVertex: Array<Vertex | null>,
    context: ParseRuntimeContext,
  ): void {
    const center = new Vector3D();
    center.set(0.0, 0.0, 0.0);
    for (let i = 0; i < n; i++) {
      center.addition(center, v[i].point);
    }
    center.inverseScaledCopy(n, center, Numeric.EPSILON_FLOAT);

    let maxD = center.distance(v[0].point);
    let max = 0;
    for (let i = 1; i < n; i++) {
      const d = center.distance(v[i].point);
      if (d > maxD) {
        maxD = d;
        max = i;
      }
    }

    const out = new Array<boolean>(MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES + 1).fill(false);
    for (let i = 0; i < n; i++) {
      out[i] = false;
    }

    let p1 = max;
    let p0 = p1 - 1;
    if (p0 < 0) {
      p0 = n - 1;
    }
    let p2 = (p1 + 1) % n;
    normal.tripleCrossProduct(v[p0].point, v[p1].point, v[p2].point);
    normal.normalize(Numeric.EPSILON_FLOAT);
    const index = normal.dominantCoordinate();

    const q = new Array<Vector2D>(MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES + 1);
    for (let i = 0; i < n; i++) {
      q[i] = new Vector2D();
      MgfVertexFaceEntitySupport.vectorProject(q[i], v[i].point, index);
    }

    let corners = n;
    const nn = new Vector3D();

    p0 = -1;
    while (corners >= 3) {
      const start = p0;
      let d = 0.0;
      let a = 0.0;
      let good: boolean;

      do {
        p0 = (p0 + 1) % n;
        while (out[p0]) {
          p0 = (p0 + 1) % n;
        }

        p1 = (p0 + 1) % n;
        while (out[p1]) {
          p1 = (p1 + 1) % n;
        }

        p2 = (p1 + 1) % n;
        while (out[p2]) {
          p2 = (p2 + 1) % n;
        }

        if (p0 === start) {
          break;
        }

        nn.tripleCrossProduct(v[p0].point, v[p1].point, v[p2].point);
        a = nn.norm();
        nn.inverseScaledCopy(a, nn, Numeric.EPSILON_FLOAT);
        d = nn.distance(normal);

        good = true;
        if (d <= 1.0 + Numeric.EPSILON) {
          for (let i = 0; i < n && good; i++) {
            if (out[i] || v[i] === v[p0] || v[i] === v[p1] || v[i] === v[p2]) {
              continue;
            }

            if (MgfVertexFaceEntitySupport.pointInsideTriangle2D(q[i], q[p0], q[p1], q[p2])) {
              good = false;
            }

            const j = (i + 1) % n;
            if (out[j] || v[j] === v[p0]) {
              continue;
            }

            if (MgfVertexFaceEntitySupport.segmentsIntersect2D(q[p2], q[p0], q[i], q[j])) {
              good = false;
            }
          }
        }
      } while (d > 1.0 + Numeric.EPSILON || !good!);

      if (p0 === start) {
        MgfEntityControl.doError("mis-built polygonal face", context);
        return;
      }

      if (globalThis.Math.abs(a) > Numeric.EPSILON) {
        // Avoid degenerate faces
        const face = MgfVertexFaceEntitySupport.newFace(v[p0], v[p1], v[p2], null, context);
        if (context.currentMaterial !== null && context.currentMaterial.isSided() === false && face !== null) {
          const twin = MgfVertexFaceEntitySupport.newFace(backVertex[p2], backVertex[p1], backVertex[p0], null, context);
          face.twin = twin;
          if (twin !== null) {
            twin.twin = face;
          }
        }
      }

      out[p1] = true;
      corners--;
    }
  }

  public static handleFaceEntity(argc: number, argv: string[], context: ParseRuntimeContext): number {
    if (argc < 4) {
      MgfEntityControl.doError("too few vertices in face", context);
      return ParseErrorContext.MGF_OK; // Don't stop parsing
    }

    if (argc - 1 > MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES) {
      MgfEntityControl.doWarning(
        "too many vertices in face. Recompile with larger MAXIMUM_FACE_VERTICES constant in read mgf",
        context,
      );
      return ParseErrorContext.MGF_OK;
    }

    if (context.inComplex === false && MgfMaterialEntitySupport.mgfMaterialChanged(context.currentMaterial, context)) {
      if (context.inSurface) {
        MgfObjectNameSupport.mgfObjectSurfaceDone(context);
      }
      MgfObjectNameSupport.mgfObjectNewSurface(context);
      const holder = [context.currentMaterial as Material];
      MgfMaterialEntitySupport.mgfGetCurrentMaterial(holder, context.singleSided, context);
      context.currentMaterial = holder[0];
      context.materialState.currentMaterial = holder[0];
    }

    const v = new Array<Vertex>(MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES + 1);
    const backV = new Array<Vertex | null>(MgfVertexFaceEntitySupport.MAXIMUM_FACE_VERTICES + 1).fill(null);

    for (let i = 0; i < argc - 1; i++) {
      const vertex = MgfVertexFaceEntitySupport.getVertex(argv[i + 1], context);
      if (vertex === null) {
        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
      }
      v[i] = vertex;
      backV[i] = null;
      if (context.currentMaterial !== null && context.currentMaterial.isSided() === false) {
        backV[i] = MgfVertexFaceEntitySupport.getBackFaceVertex(v[i], context);
      }
    }

    const normal = new Vector3D();
    if (MgfVertexFaceEntitySupport.faceNormal(argc - 1, v, normal) === null) {
      MgfEntityControl.doWarning("degenerate face", context);
      return ParseErrorContext.MGF_OK; // Ignore generated face
    }

    let errorCode = ParseErrorContext.MGF_OK;
    let face: Patch | null;
    let twin: Patch | null;

    if (argc === 4) {
      // Triangles
      face = MgfVertexFaceEntitySupport.newFace(v[0], v[1], v[2], null, context);
      if (context.currentMaterial !== null && context.currentMaterial.isSided() === false && face !== null) {
        twin = MgfVertexFaceEntitySupport.newFace(backV[2], backV[1], backV[0], null, context);
        face.twin = twin;
        if (twin !== null) {
          twin.twin = face;
        }
      }
    }
    else if (argc === 5) {
      // Quadrilaterals
      if (context.inComplex || MgfVertexFaceEntitySupport.faceIsConvex(argc - 1, v, normal)) {
        face = MgfVertexFaceEntitySupport.newFace(v[0], v[1], v[2], v[3], context);
        if (context.currentMaterial !== null && context.currentMaterial.isSided() === false && face !== null) {
          twin = MgfVertexFaceEntitySupport.newFace(backV[3], backV[2], backV[1], backV[0], context);
          face.twin = twin;
          if (twin !== null) {
            twin.twin = face;
          }
        }
      }
      else {
        MgfVertexFaceEntitySupport.doComplexFace(argc - 1, v, normal, backV, context);
        errorCode = ParseErrorContext.MGF_OK;
      }
    }
    else {
      // More than 4 vertices
      MgfVertexFaceEntitySupport.doComplexFace(argc - 1, v, normal, backV, context);
      errorCode = ParseErrorContext.MGF_OK;
    }

    return errorCode;
  }

  public static handleSurfaceEntity(argc: number, argv: string[], context: ParseRuntimeContext): number {
    if (context.inComplex) {
      // mgfEntitySphere calls mgfEntityCone
      return MgfVertexFaceEntitySupport.doDiscreteConic(argc, argv, context);
    }
    context.inComplex = true;
    context.geometryBuildState.inComplex = true;
    if (context.inSurface) {
      MgfObjectNameSupport.mgfObjectSurfaceDone(context);
    }
    MgfObjectNameSupport.mgfObjectNewSurface(context);
    const holder = [context.currentMaterial as Material];
    MgfMaterialEntitySupport.mgfGetCurrentMaterial(holder, context.singleSided, context);
    context.currentMaterial = holder[0];
    context.materialState.currentMaterial = holder[0];

    const errcode = MgfVertexFaceEntitySupport.doDiscreteConic(argc, argv, context);

    MgfObjectNameSupport.mgfObjectSurfaceDone(context);
    context.inComplex = false;
    context.geometryBuildState.inComplex = false;

    return errcode;
  }

  /**
  Eliminates holes by delegating to seam generator.
  */
  public static handleFaceWithHolesEntity(argc: number, argv: string[], context: ParseRuntimeContext): number {
    return MgfFaceWithHolesEntityExpander.handleEntity(argc, argv, context);
  }

  /**
  Handle a vertex entity.
  */
  public static handleVertexEntity(ac: number, av: string[], context: ParseRuntimeContext): number {
    let lp: LookUpEntity<VertexContext> | null;
    let currentVertexContext = context.vertexRepository.currentVertex;

    switch (MgfEntityControl.mgfEntity(av[0], context)) {
      case EntityTypeContext.VERTEX:
        // Get/set vertex context
        if (ac > 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (ac === 1) {
          // Set unnamed vertex context
          context.vertexRepository.unNamedVertexContext.copy(context.vertexRepository.defaultVertexContext);
          currentVertexContext = context.vertexRepository.unNamedVertexContext;
          context.vertexRepository.currentVertex = currentVertexContext;
          context.currentVertexName = null;
          context.geometryBuildState.currentVertexName = null;
          return ParseErrorContext.MGF_OK;
        }
        if (TokenValidationContext.isName(av[1]) === false) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        lp = (context.vertexRepository.vertexLookUpTable as any).lookUpFind(av[1]);
        // Lookup context
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        context.currentVertexName = lp.key;
        context.geometryBuildState.currentVertexName = context.currentVertexName;
        currentVertexContext = lp.data;
        context.vertexRepository.currentVertex = currentVertexContext;
        if (ac === 2) {
          // Re-establish previous context
          if (currentVertexContext === null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
          }
          return ParseErrorContext.MGF_OK;
        }
        if (av[2].length !== 1 || av[2].charAt(0) !== "=") {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (currentVertexContext === null) {
          // Create new vertex context
          context.currentVertexName = av[1];
          context.geometryBuildState.currentVertexName = context.currentVertexName;
          lp.key = context.currentVertexName;
          currentVertexContext = new VertexContext();
          lp.data = currentVertexContext;
          context.vertexRepository.currentVertex = currentVertexContext;
        }
        if (ac === 3) {
          // Use default template
          currentVertexContext.copy(context.vertexRepository.defaultVertexContext);
          return ParseErrorContext.MGF_OK;
        }
        lp = (context.vertexRepository.vertexLookUpTable as any).lookUpFind(av[3]);
        // Lookup template
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        if (lp.data === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        currentVertexContext.copy(lp.data);
        currentVertexContext.clock++;
        return ParseErrorContext.MGF_OK;
      case EntityTypeContext.MGF_POINT:
        // Set point
        if (ac !== 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (
          TokenValidationContext.isFloat(av[1]) === false
          || TokenValidationContext.isFloat(av[2]) === false
          || TokenValidationContext.isFloat(av[3]) === false
        ) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (currentVertexContext === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        currentVertexContext.p.x = Number.parseFloat(av[1]);
        currentVertexContext.p.y = Number.parseFloat(av[2]);
        currentVertexContext.p.z = Number.parseFloat(av[3]);
        currentVertexContext.clock++;
        return ParseErrorContext.MGF_OK;
      case EntityTypeContext.MGF_NORMAL:
        // Set normal
        if (ac !== 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (
          TokenValidationContext.isFloat(av[1]) === false
          || TokenValidationContext.isFloat(av[2]) === false
          || TokenValidationContext.isFloat(av[3]) === false
        ) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (currentVertexContext === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        currentVertexContext.n.x = Number.parseFloat(av[1]);
        currentVertexContext.n.y = Number.parseFloat(av[2]);
        currentVertexContext.n.z = Number.parseFloat(av[3]);
        currentVertexContext.n.normalizeAndGivePreviousNorm(Numeric.EPSILON);
        currentVertexContext.clock++;
        return ParseErrorContext.MGF_OK;
      default:
        break;
    }
    return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
  }

  /**
  Get a named vertex.
  */
  public static getNamedVertex(name: string, context: ParseRuntimeContext): VertexContext | null {
    const lp = (context.vertexRepository.vertexLookUpTable as any).lookUpFind(name) as LookUpEntity<VertexContext> | null;
    if (lp === null) {
      return null;
    }
    return lp.data;
  }

  public static initGeometryContextTables(context: ParseRuntimeContext): void {
    context.vertexRepository.reset();
    context.currentVertexName = null;
    context.geometryBuildState.currentVertexName = null;
  }
}
