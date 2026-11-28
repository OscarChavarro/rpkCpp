import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Vector2Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Dd";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Cone } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/Cone";
import { Prism } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/Prism";
import { Sphere } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/Sphere";
import { Torus } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/Torus";
import { Tube } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/Tube";
import { Numeric } from "../../skin/Numeric";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import type { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ReaderContext } from "../context/ReaderContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import type { VertexContext } from "../context/VertexContext";
import { MgfBoundaryFaceEmitter } from "./MgfBoundaryFaceEmitter";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfSurfaceNormalModel } from "./MgfSurfaceNormalModel";
import { MgfTessellationMath } from "./MgfTessellationMath";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

/**
Handlers of the MGF surface primitives (`sph`, `cyl`, `cone`, `torus`,
`ring` and `prism`) that build each one as the matching VITRAL geometry
(`Sphere`, `Cone`, `Torus`, `Tube.createWasherLamina` and `Prism`),
export it to a `PolyhedralBoundedSolid` and turn its faces into MGF faces
(see `MgfBoundaryFaceEmitter`).

The tessellation follows the classic MGF one: 4 * n divisions around each
circle and 2 * n along the meridians of the sphere, for n quarter circle
divisions, quadrilaterals with triangle fans at the poles and apexes, open
cones and cylinders, and the vertex normals of `MgfSurfaceNormalModel`.
The local frame of each primitive is the one of the classic tessellation
(`MgfTessellationMath.mgfMakeAxes` on the axis of the primitive), so the
vertices are at the same angles.

VITRAL primitives have positive sizes: an MGF primitive with negative radii
(whose surface faces inwards) is built with the absolute values and its
faces are turned inside out.
*/
export class MgfVitralPrimitiveTessellator {
  private constructor() {
  }

  private static circleDivisions(context: ParseRuntimeContext): number {
    return 4 * Math.max(1, context.numberOfQuarterCircleDivisions);
  }

  private static makeAxes(w: Vector3Dd): Vector3Dd[] {
    const u: Vector3Dd[] = [new Vector3Dd(0.0, 0.0, 0.0)];
    const v: Vector3Dd[] = [new Vector3Dd(0.0, 0.0, 0.0)];
    MgfTessellationMath.mgfMakeAxes(u, v, w, VSDK.EPSILON);
    return [u[0]!, v[0]!];
  }

  private static roundedToZero(value: number): number {
    const holder = [value];
    Numeric.roundDeltaToZero(holder, VSDK.EPSILON);
    return holder[0]!;
  }

  /**
  sph center radius
  */
  public static handleSphere(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    if (argumentCount !== 3) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context);
    if (center === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const radius = Number.parseFloat(argumentValues[2]!);
    if (Math.abs(radius) <= VSDK.EPSILON) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // As the classic tessellation, the axis of the sphere is +Z
    const w = new Vector3Dd(0.0, 0.0, 1.0);
    const [u, v] = MgfVitralPrimitiveTessellator.makeAxes(w);

    const n = MgfVitralPrimitiveTessellator.circleDivisions(context);
    const sphere = new Sphere(Math.abs(radius));
    const solid = sphere.exportToPolyhedralBoundedSolid(n, Math.trunc(n / 2), true);
    const emitter = new MgfBoundaryFaceEmitter(center.p, u!, v!, w, radius < 0.0);
    return emitter.emitSolid(solid, MgfSurfaceNormalModel.sphere(), context);
  }

  /**
  cyl bottomCenter radius topCenter: a cone with equal radii
  */
  public static handleCylinder(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const coneArguments: string[] = [
      context.entityNames[EntityTypeContext.CONE]!,
      argumentValues[1]!,
      argumentValues[2]!,
      argumentValues[3]!,
      argumentValues[2]!,
    ];
    return MgfVitralPrimitiveTessellator.handleCone(5, coneArguments, context);
  }

  /**
  cone center1 radius1 center2 radius2: the open lateral surface of a cone.
  Radii of opposite signs are illegal, one radius may be 0 (the apex) and
  negative radii turn the surface inwards
  */
  public static handleCone(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    if (argumentCount !== 5) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const v1Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context);
    const v2Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[3]!, context);
    if (v1Context === null || v2Context === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!) || !TokenValidationContext.isFloat(argumentValues[4]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const radius1 = MgfVitralPrimitiveTessellator.roundedToZero(Number.parseFloat(argumentValues[2]!));
    const radius2 = MgfVitralPrimitiveTessellator.roundedToZero(Number.parseFloat(argumentValues[4]!));

    if (radius1 === 0.0 && radius2 === 0.0) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (radius1 !== 0.0 && radius2 !== 0.0 && ((radius1 < 0.0) !== (radius2 < 0.0))) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // The base of the VITRAL cone is the end with the non zero radius,
    // the classic tessellation took the end 2 when it could
    if (radius2 === 0.0) {
      return MgfVitralPrimitiveTessellator.emitCone(v1Context.p, radius1, v2Context.p, radius2, context);
    }
    return MgfVitralPrimitiveTessellator.emitCone(v2Context.p, radius2, v1Context.p, radius1, context);
  }

  private static emitCone(
    bottomCenter: Vector3Dd,
    bottomRadius: number,
    topCenter: Vector3Dd,
    topRadius: number,
    context: ParseRuntimeContext): number {
    const axis: Vector3Dd[] = [new Vector3Dd(
      topCenter.x() - bottomCenter.x(),
      topCenter.y() - bottomCenter.y(),
      topCenter.z() - bottomCenter.z())];
    const height = MgfTessellationMath.normalizeAndGivePreviousNorm(axis, VSDK.EPSILON);
    if (Numeric.doubleEqual(height, 0.0, VSDK.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    const w = axis[0]!;
    const [u, v] = MgfVitralPrimitiveTessellator.makeAxes(w);

    const inwards = bottomRadius < 0.0;
    const bottom = Math.abs(bottomRadius);
    const top = Math.abs(topRadius);
    const cone = new Cone(bottom, top, height);
    const solid = cone.exportToPolyhedralBoundedSolid(MgfVitralPrimitiveTessellator.circleDivisions(context), 1, false);
    const emitter = new MgfBoundaryFaceEmitter(bottomCenter, u!, v!, w, inwards);
    return emitter.emitSolid(solid, MgfSurfaceNormalModel.coneLamina(bottom, top, height), context);
  }

  /**
  torus center minorRadius maxRadius: the center vertex gives the axis as its
  normal. The distance from the axis goes from the first radius to the second
  one; both negative turn the surface inwards
  */
  public static handleTorus(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context);
    if (center === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (MgfTessellationMath.isNull(center.n, VSDK.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!) || !TokenValidationContext.isFloat(argumentValues[3]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const minRadius = MgfVitralPrimitiveTessellator.roundedToZero(Number.parseFloat(argumentValues[2]!));
    const maxRadius = Number.parseFloat(argumentValues[3]!);
    if (minRadius === 0.0 || (minRadius > 0.0 ? maxRadius - minRadius : minRadius - maxRadius) <= 0.0) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    const [u, v] = MgfVitralPrimitiveTessellator.makeAxes(center.n);

    const majorRadius = Math.abs(0.5 * (minRadius + maxRadius));
    const tubeRadius = Math.abs(0.5 * (maxRadius - minRadius));
    const n = MgfVitralPrimitiveTessellator.circleDivisions(context);
    const torus = new Torus(majorRadius, tubeRadius);
    const solid = torus.exportToPolyhedralBoundedSolid(n, n);
    const emitter = new MgfBoundaryFaceEmitter(center.p, u!, v!, center.n, minRadius < 0.0);
    return emitter.emitSolid(solid, MgfSurfaceNormalModel.torus(majorRadius), context);
  }

  /**
  ring center innerRadius outerRadius: a planar disk (inner radius 0) or
  annulus facing the normal of the center vertex
  */
  public static handleRing(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context);
    if (center === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (MgfTessellationMath.isNull(center.n, VSDK.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!) || !TokenValidationContext.isFloat(argumentValues[3]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const minRadius = MgfVitralPrimitiveTessellator.roundedToZero(Number.parseFloat(argumentValues[2]!));
    const maxRadius = Number.parseFloat(argumentValues[3]!);
    if (minRadius < 0.0 || maxRadius <= minRadius) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    const [u, v] = MgfVitralPrimitiveTessellator.makeAxes(center.n);

    const innerRadius = Numeric.doubleEqual(minRadius, 0.0, VSDK.EPSILON) ? 0.0 : minRadius;
    const solid = Tube.createWasherLamina(
      innerRadius, maxRadius, 0.0, MgfVitralPrimitiveTessellator.circleDivisions(context), true);
    const emitter = new MgfBoundaryFaceEmitter(center.p, u!, v!, center.n, false);
    return emitter.emitSolid(solid, MgfSurfaceNormalModel.flatLamina(), context);
  }

  /**
  prism v1 v2 ... vn length: the polygon v1 .. vn (counterclockwise around its
  normal) extruded `length` units against its normal; a negative length
  extrudes it along the normal, with the surface facing inwards. The walls
  take the normals of the vertices of the polygon, if any of them has one
  */
  public static handlePrism(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const maximumVertices = ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2;
    const polygon: VertexContext[] = [];

    if (argumentCount < 5 || argumentCount - 2 > maximumVertices) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if (!TokenValidationContext.isFloat(argumentValues[argumentCount - 1]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const length = Number.parseFloat(argumentValues[argumentCount - 1]!);
    if (length <= VSDK.EPSILON && length >= -VSDK.EPSILON) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Normal of the polygon, as the classic tessellation
    const numberOfVertices = argumentCount - 2;
    let hasNormals = false;
    let normal = new Vector3Dd(0.0, 0.0, 0.0);
    let previous = new Vector3Dd(0.0, 0.0, 0.0);
    for (let i = 0; i < numberOfVertices; i++) {
      const vertex = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i + 1]!, context);
      if (vertex === null) {
        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
      }
      polygon[i] = vertex;
      if (i === 0) {
        continue;
      }
      if (!MgfTessellationMath.isNull(vertex.n, VSDK.EPSILON)) {
        hasNormals = true;
      }
      const current = vertex.p.subtract(polygon[0]!.p);
      normal = normal.add(previous.crossProduct(current));
      previous = current;
    }
    const normalHolder = [normal];
    if (MgfTessellationMath.normalizeAndGivePreviousNorm(normalHolder, VSDK.EPSILON) === 0.0) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    normal = normalHolder[0]!;

    // Local frame: the polygon on z = 0, extruded along +Z
    const w = length > 0.0 ? normal.multiply(-1.0) : normal;
    const [u, v] = MgfVitralPrimitiveTessellator.makeAxes(w);
    const emitter = new MgfBoundaryFaceEmitter(polygon[0]!.p, u!, v!, w, length < 0.0);

    const base = new ArrayList<Vector2Dd>();
    for (let i = 0; i < numberOfVertices; i++) {
      const d = polygon[i]!.p.subtract(polygon[0]!.p);
      base.add(new Vector2Dd(d.dotProduct(u!), d.dotProduct(v!)));
    }
    const prism = new Prism(base, Math.abs(length));
    if (!prism.hasValidParameters()) {
      MgfEntityControl.doWarning("prism base is not a simple polygon", context);
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    const solid = prism.exportToPolyhedralBoundedSolid();

    const points: Vector3Dd[] = [];
    const normals: Vector3Dd[] = [];
    let errorCode = ParseErrorContext.MGF_OK;
    for (let f = 0; f < solid.getPolygonsList().size() && errorCode === ParseErrorContext.MGF_OK; f++) {
      const faceSize = MgfBoundaryFaceEmitter.faceVertices(solid, f, points, maximumVertices);
      if (faceSize < 3) {
        errorCode = ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        break;
      }
      const faceNormal = MgfBoundaryFaceEmitter.polygonNormal(faceSize, points);
      const wall = Math.abs(faceNormal.z()) < 0.5 * faceNormal.length();
      let faceNormals: Vector3Dd[] | null = null;
      if (hasNormals && wall) {
        // Each wall vertex takes the normal of its polygon vertex
        for (let k = 0; k < faceSize; k++) {
          let closest = 0;
          let closestDistance = -1.0;
          for (let i = 0; i < numberOfVertices; i++) {
            const dx = points[k]!.x() - base.get(i)!.x;
            const dy = points[k]!.y() - base.get(i)!.y;
            const distance = dx * dx + dy * dy;
            if (closestDistance < 0.0 || distance < closestDistance) {
              closest = i;
              closestDistance = distance;
            }
          }
          // The classic tessellation kept them as given, also when
          // the faces are reversed (and so their normals negated)
          normals[k] = length < 0.0 ? polygon[closest]!.n.multiply(-1.0) : polygon[closest]!.n;
        }
        faceNormals = normals;
      }
      errorCode = emitter.emitFace(faceSize, points, faceNormals, context);
    }
    return errorCode;
  }
}
