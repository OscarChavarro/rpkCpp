import { Math as JavaMath } from "../../../../../../java/lang/Math.js";
import type { Matrix4x4d } from "../../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidEulerOperators } from "./PolyhedralBoundedSolidEulerOperators.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import { PolyhedralBoundedSolidValidationEngine } from "./PolyhedralBoundedSolidValidationEngine.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";

/**
Constructions of [MANT1988] chapter 12 composed only of Euler operators: arcs
and circular laminas (`mvfs`, `smev`, `smef`, section 12.2) and translational
sweeps of a face (`lmev`, `lmef`, section 12.3.1). Solids that model their own
boundary representation (`Cone`, `Arrow`) build on them.
*/
export class PolyhedralBoundedSolidEulerConstructions {
    /** Java 17 `Math.toRadians(double)`: `angdeg * DEGREES_TO_RADIANS`. */
    private static readonly DEGREES_TO_RADIANS = 0.017453292519943295;

    /**
    [MANT1988] section 12.2, program 12.1: appends `n` vertices of an arc on
    plane `z = height`, centered at (`cx`, `cy`), from `phi1` to `phi2`
    (degrees, counterclockwise, 0 degrees on +X), starting at vertex
    `vertexId` of face `faceId`.
    */
    public static addArcToExistingFace(
        solid: PolyhedralBoundedSolid,
        faceId: number,
        vertexId: number,
        cx: number,
        cy: number,
        radius: number,
        height: number,
        phi1: number,
        phi2: number,
        n: number,
    ): void {
        let x: number;
        let y: number;
        let angle: number;
        let prev: number;
        let i: number;
        let nextVertexId: number;

        angle = phi1 * PolyhedralBoundedSolidEulerConstructions.DEGREES_TO_RADIANS;
        const inc = ((phi2 - phi1) / n) * PolyhedralBoundedSolidEulerConstructions.DEGREES_TO_RADIANS;
        prev = vertexId;
        for (i = 0; i < n; i++) {
            angle += inc;
            // Snap trig values to 1e-10 grid so that equal angular positions on
            // two separately constructed circles always produce bit-identical
            // coordinates, preventing near-coincident vertices downstream.
            // Java `Math.round(double)` rounds half toward positive infinity,
            // as ECMAScript `Math.round`, but returns a `long`: a value that
            // rounds to zero gives +0.0, never -0.0 (see `JavaMath.round`).
            x = JavaMath.round((cx + radius * Math.cos(angle)) * 1.0e10) / 1.0e10;
            y = JavaMath.round((cy + radius * Math.sin(angle)) * 1.0e10) / 1.0e10;
            nextVertexId = solid.getMaxVertexId() + 1;
            PolyhedralBoundedSolidEulerOperators.smev(solid, faceId, prev, nextVertexId, new Vector3Dd(x, y, height));
            prev = nextVertexId;
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }

    /**
    [MANT1988] section 12.2, program 12.2: a planar circular lamina of `n`
    vertices as a single face (`mvfs`, arc, `smef`).
    @return a new solid, owned by the caller
    */
    public static createCircularLamina(
        cx: number,
        cy: number,
        rad: number,
        h: number,
        n: number,
    ): PolyhedralBoundedSolid {
        const solid = new PolyhedralBoundedSolid();
        PolyhedralBoundedSolidEulerOperators.mvfs(solid, new Vector3Dd(cx + rad, cy, h), 1, 1);
        PolyhedralBoundedSolidEulerConstructions.addArcToExistingFace(
            solid,
            1,
            1,
            cx,
            cy,
            rad,
            h,
            0,
            ((n - 1) * 360.0) / n,
            n - 1,
        );
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, n, 1, 2);
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }

    /**
    Generalized translational sweep of a face, inspired by [MANT1988] 12.3.1.

    Instead of pure translation, transform `T` can include translation,
    rotation, and scale in the face-local context.

    PRE: `face` is closed and planar.
    */
    public static translationalSweepExtrudeFace(
        solid: PolyhedralBoundedSolid,
        face: _PolyhedralBoundedSolidFace | null,
        transformationMatrix: Matrix4x4d,
    ): void {
        let l: _PolyhedralBoundedSolidLoop;
        let first: _PolyhedralBoundedSolidHalfEdge;
        let scan: _PolyhedralBoundedSolidHalfEdge;
        let v: _PolyhedralBoundedSolidVertex;
        let newPos: Vector3Dd;
        let i: number;

        for (i = 0; i < face!.boundariesList.size(); i++) {
            l = face!.boundariesList.get(i)!;
            first = l.boundaryStartHalfEdge!;
            scan = first.next()!;
            v = scan.startingVertex;
            newPos = transformationMatrix.multiply(v.position);
            PolyhedralBoundedSolidEulerOperators.lmev(solid, scan, scan, solid.getMaxVertexId() + 1, newPos);
            while (scan !== first) {
                v = scan.next()!.startingVertex;
                newPos = transformationMatrix.multiply(v.position);
                PolyhedralBoundedSolidEulerOperators.lmev(
                    solid,
                    scan.next(),
                    scan.next(),
                    solid.getMaxVertexId() + 1,
                    newPos,
                );
                PolyhedralBoundedSolidEulerOperators.lmef(
                    solid,
                    scan.previous(),
                    scan.next()!.next(),
                    solid.getMaxFaceId() + 1,
                );
                scan = scan.next()!.mirrorHalfEdge()!.next()!;
            }
            PolyhedralBoundedSolidEulerOperators.lmef(
                solid,
                scan.previous(),
                scan.next()!.next(),
                solid.getMaxFaceId() + 1,
            );
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }

    /**
    Variant of `translationalSweepExtrudeFace` with an extra planar check pass.

    After sweep generation, each created side face is tested for planarity.
    If a face is non-planar, it is split once (`lmef`) to triangulate it.
    */
    public static translationalSweepExtrudeFacePlanar(
        solid: PolyhedralBoundedSolid,
        face: _PolyhedralBoundedSolidFace | null,
        transformationMatrix: Matrix4x4d,
    ): void {
        let l: _PolyhedralBoundedSolidLoop;
        let first: _PolyhedralBoundedSolidHalfEdge;
        let scan: _PolyhedralBoundedSolidHalfEdge;
        let v: _PolyhedralBoundedSolidVertex;
        let newPos: Vector3Dd;
        const newFaces: number[] = [];
        let i: number;
        let newFaceId: number;

        for (i = 0; i < face!.boundariesList.size(); i++) {
            l = face!.boundariesList.get(i)!;
            first = l.boundaryStartHalfEdge!;
            scan = first.next()!;
            v = scan.startingVertex;
            newPos = transformationMatrix.multiply(v.position);
            PolyhedralBoundedSolidEulerOperators.lmev(solid, scan, scan, solid.getMaxVertexId() + 1, newPos);
            while (scan !== first) {
                v = scan.next()!.startingVertex;
                newPos = transformationMatrix.multiply(v.position);
                PolyhedralBoundedSolidEulerOperators.lmev(
                    solid,
                    scan.next(),
                    scan.next(),
                    solid.getMaxVertexId() + 1,
                    newPos,
                );
                newFaceId = solid.getMaxFaceId() + 1;
                PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next()!.next(), newFaceId);
                newFaces.push(newFaceId);
                scan = scan.next()!.mirrorHalfEdge()!.next()!;
            }
            newFaceId = solid.getMaxFaceId() + 1;
            PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next()!.next(), newFaceId);
            newFaces.push(newFaceId);
        }

        let newFace: _PolyhedralBoundedSolidFace;
        for (i = 0; i < newFaces.length; i++) {
            newFaceId = newFaces[i]!;
            newFace = solid.findFace(newFaceId)!;
            if (!PolyhedralBoundedSolidGeometricValidator.validateFaceIsPlanar(newFace)) {
                scan = newFace.boundariesList.get(0)!.boundaryStartHalfEdge!;
                newFaceId = solid.getMaxFaceId() + 1;
                PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.next(), scan.previous(), newFaceId);
            }
        }

        while (newFaces.length !== 0) {
            newFaces.splice(0, 1);
        }

        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }
}
