import type { GeometryBounds } from "../element/GeometryBounds.js";
import { Double } from "../../../../../java/lang/Double.js";
import { PolyhedralBoundedSolidEulerConstructions } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.js";
//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { VSDK } from "../../../common/VSDK.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import type { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import type { PolyhedralBoundedSolid } from "./polyhedralBoundedSolid/PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidValidationEngine } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.js";
import { Tube } from "./Tube.js";
import { Solid } from "./Solid.js";

export class Cone extends Solid {
    private static readonly NO_HIT = Number.POSITIVE_INFINITY;

    private bottomRadius: number; // Radius at the base
    private topRadius: number; // Radius at the top
    private height: number; // Height

    private static readonly DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    private static readonly DEFAULT_HEIGHT_DIVISIONS = 1;
    private static readonly MIN_CIRCUMFERENCE_DIVISIONS = 3;
    private static readonly MIN_HEIGHT_DIVISIONS = 1;

    public constructor(bottomRadius: number, topRadius: number, height: number) {
        super();
        this.bottomRadius = bottomRadius;
        this.topRadius = topRadius;
        this.height = height;
        this.getControlSpecifications().push("double;bottomRadius;(0, INFINITE)");
        this.getControlSpecifications().push("double;topRadius;[0, INFINITE)");
        this.getControlSpecifications().push("double;height;(0, INFINITE)");
    }

    public getBottomRadius(): number {
        return this.bottomRadius;
    }

    public getTopRadius(): number {
        return this.topRadius;
    }

    public getHeight(): number {
        return this.height;
    }

    public setBottomRadius(value: number): void {
        this.bottomRadius = value;
    }

    public setTopRadius(value: number): void {
        this.topRadius = value;
    }

    public setHeight(value: number): void {
        this.height = value;
    }

    /**
    Java overloads `doIntersectionCylinder(Ray)` (always null) and
    `doIntersectionCylinder(Ray, double, double, RayHit)`.
    */
    private doIntersectionCylinder(inOutRay: Ray, inR?: number, inH?: number, outInfo?: RayHit | null): Ray | null {
        if (inR === undefined || inH === undefined || outInfo === undefined) {
            return null;
        }
        let A: number;
        let B: number;
        let C: number;
        let discriminant: number;
        let t0: number;
        const ox = inOutRay.getOrigin().x();
        const oy = inOutRay.getOrigin().y();
        const oz = inOutRay.getOrigin().z();
        const dx = inOutRay.getDirection().x();
        const dy = inOutRay.getDirection().y();
        const dz = inOutRay.getDirection().z();

        //- Calcula el termino A --------------------------------------------
        A = VSDK.square(dx) + VSDK.square(dy);
        if (Math.abs(A) <= VSDK.EPSILON) return null;

        //- Calcula el termino B --------------------------------------------
        B = 2 * (dx * ox + dy * oy);

        //- Calcula el termino C --------------------------------------------
        C = VSDK.square(ox) + VSDK.square(oy) - VSDK.square(inR);

        //- Calcula el discriminant. Si el discriminant no es positivo el -
        //- rayo no intersecta el cilindro. retorna t = 0
        discriminant = VSDK.square(B) - 4 * A * C;
        if (discriminant <= VSDK.EPSILON) return null;

        //- Resuelve la ecuacion cuadratica para las raices de la ecuacion. -
        //- (-B +/- sqrt(B^2 - 4*A*C)) / 2A.                                -
        discriminant = Math.sqrt(discriminant);
        t0 = (-B - discriminant) / (2 * A);

        //- Si t0 es > 0 listo. Si no debemos calcular la otra raiz t1. -----
        if (t0 > VSDK.EPSILON) {
            const pz = oz + dz * t0;
            if (pz > inH || pz < 0) {
                return null;
            }

            if (outInfo !== null) {
                const px = ox + dx * t0;
                const py = oy + dy * t0;
                outInfo.point = new Vector3Dd(px, py, pz);
                outInfo.normal = new Vector3Dd(px, py, 0).normalized();
            }
            return inOutRay.withT(t0);
        }

        return null;
    }

    private doIntersectionCone(inOutRay: Ray, inR: number, inH: number, outInfo: RayHit | null): Ray | null {
        let A: number;
        let B: number;
        let C: number;
        let discriminant: number;
        let t0: number;
        const ox = inOutRay.getOrigin().x();
        const oy = inOutRay.getOrigin().y();
        const oz = inOutRay.getOrigin().z();
        const dx = inOutRay.getDirection().x();
        const dy = inOutRay.getDirection().y();
        const dz = inOutRay.getDirection().z();
        if (inH <= VSDK.EPSILON) {
            return null;
        }
        const shiftedOz = oz - inH;
        const ratio = inR / inH;
        const ratioSquared = VSDK.square(ratio);

        //- Calcula el termino A --------------------------------------------
        A = VSDK.square(dx) + VSDK.square(dy) - VSDK.square(dz * ratio);
        if (Math.abs(A) <= VSDK.EPSILON) return null;

        //- Calcula el termino B --------------------------------------------
        B = 2 * (dx * ox + dy * oy - dz * shiftedOz * ratioSquared);

        //- Calcula el termino C --------------------------------------------
        C = VSDK.square(ox) + VSDK.square(oy) - VSDK.square(shiftedOz * ratio);

        //- Calcula el discriminant. Si el discriminant no es positivo el -
        //- rayo no intersecta la esfera. retorna t = 0
        discriminant = VSDK.square(B) - 4 * A * C;
        if (discriminant <= VSDK.EPSILON) return null;

        //- Resuelve la ecuacion cuadratica para las raices de la ecuacion. -
        //- (-B +/- sqrt(B^2 - 4*A*C)) / 2A.                                -
        discriminant = Math.sqrt(discriminant);
        t0 = (-B - discriminant) / (2 * A);

        //- Si t0 es > 0 listo. Si no debemos calcular la otra raiz t1. -----
        if (t0 > VSDK.EPSILON) {
            const shiftedPz = shiftedOz + dz * t0;
            if (shiftedPz > 0 || shiftedPz < -inH) {
                return null;
            }

            if (outInfo !== null) {
                const px = ox + dx * t0;
                const py = oy + dy * t0;
                outInfo.point = new Vector3Dd(px, py, shiftedPz + inH);
                outInfo.normal = new Vector3Dd(px, py, -shiftedPz * ratioSquared).normalized();
            }
            return inOutRay.withT(t0);
        }

        return null;
    }

    private doIntersectionTap(inOutRay: Ray, inR: number, inH: number, outInfo: RayHit | null): Ray | null {
        const dx = inOutRay.getDirection().x();
        const dy = inOutRay.getDirection().y();
        const dz = inOutRay.getDirection().z();
        const ox = inOutRay.getOrigin().x();
        const oy = inOutRay.getOrigin().y();
        const oz = inOutRay.getOrigin().z();

        if (Math.abs(dz) > VSDK.EPSILON) {
            const t = (inH - oz) / dz;
            if (t > VSDK.EPSILON) {
                const px = ox + dx * t;
                const py = oy + dy * t;
                if (VSDK.square(px) + VSDK.square(py) < VSDK.square(inR)) {
                    if (outInfo !== null) {
                        outInfo.normal = new Vector3Dd(0, 0, 1);
                        outInfo.point = new Vector3Dd(px, py, inH);
                    }
                    return inOutRay.withT(t);
                }
            }
        }
        return null;
    }

    private static hasHit(t: number): boolean {
        return t !== Cone.NO_HIT;
    }

    private doIntersectionCylinderDistance(inOutRay: Ray, inR: number, inH: number): number {
        const dx = inOutRay.getDirection().x();
        const dy = inOutRay.getDirection().y();
        const dz = inOutRay.getDirection().z();
        const ox = inOutRay.getOrigin().x();
        const oy = inOutRay.getOrigin().y();
        const oz = inOutRay.getOrigin().z();
        const A = VSDK.square(dx) + VSDK.square(dy);
        if (Math.abs(A) <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const B = 2 * (dx * ox + dy * oy);
        const C = VSDK.square(ox) + VSDK.square(oy) - VSDK.square(inR);
        const discriminant = VSDK.square(B) - 4 * A * C;
        if (discriminant <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const t0 = (-B - Math.sqrt(discriminant)) / (2 * A);
        if (t0 <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }
        const pz = oz + dz * t0;
        if (pz > inH || pz < 0) {
            return Cone.NO_HIT;
        }
        return t0;
    }

    private doIntersectionConeDistance(inOutRay: Ray, inR: number, inH: number): number {
        if (inH <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const dx = inOutRay.getDirection().x();
        const dy = inOutRay.getDirection().y();
        const dz = inOutRay.getDirection().z();
        const ox = inOutRay.getOrigin().x();
        const oy = inOutRay.getOrigin().y();
        const shiftedOz = inOutRay.getOrigin().z() - inH;
        const ratio = inR / inH;
        const ratioSquared = VSDK.square(ratio);
        const A = VSDK.square(dx) + VSDK.square(dy) - VSDK.square(dz * ratio);
        if (Math.abs(A) <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const B = 2 * (dx * ox + dy * oy - dz * shiftedOz * ratioSquared);
        const C = VSDK.square(ox) + VSDK.square(oy) - VSDK.square(shiftedOz * ratio);
        const discriminant = VSDK.square(B) - 4 * A * C;
        if (discriminant <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const t0 = (-B - Math.sqrt(discriminant)) / (2 * A);
        if (t0 <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }
        const shiftedPz = shiftedOz + dz * t0;
        if (shiftedPz > 0 || shiftedPz < -inH) {
            return Cone.NO_HIT;
        }
        return t0;
    }

    private doIntersectionTapDistance(inOutRay: Ray, inR: number, inH: number): number {
        const dz = inOutRay.getDirection().z();
        if (Math.abs(dz) <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const t = (inH - inOutRay.getOrigin().z()) / dz;
        if (t <= VSDK.EPSILON) {
            return Cone.NO_HIT;
        }

        const px = inOutRay.getOrigin().x() + inOutRay.getDirection().x() * t;
        const py = inOutRay.getOrigin().y() + inOutRay.getDirection().y() * t;
        if (VSDK.square(px) + VSDK.square(py) >= VSDK.square(inR)) {
            return Cone.NO_HIT;
        }
        return t;
    }

    private doIntersectionDistanceOnly(inOutRay: Ray, outHit: RayHit | null): boolean {
        let winnerT = Cone.NO_HIT;

        if (this.topRadius < VSDK.EPSILON && this.bottomRadius > VSDK.EPSILON) {
            const bodyT = this.doIntersectionConeDistance(inOutRay, this.bottomRadius, this.height);
            const tap1T = this.doIntersectionTapDistance(inOutRay, this.bottomRadius, 0);
            if (Cone.hasHit(tap1T) && (!Cone.hasHit(bodyT) || tap1T < bodyT)) {
                winnerT = tap1T;
            } else if (Cone.hasHit(bodyT)) {
                winnerT = bodyT;
            }
        } else if (VSDK.equals(this.bottomRadius, this.topRadius)) {
            const bodyT = this.doIntersectionCylinderDistance(inOutRay, this.bottomRadius, this.height);
            const tap1T = this.doIntersectionTapDistance(inOutRay, this.bottomRadius, 0);
            const tap2T = this.doIntersectionTapDistance(inOutRay, this.bottomRadius, this.height);

            if (
                Cone.hasHit(bodyT) &&
                ((Cone.hasHit(tap1T) && bodyT < tap1T) || !Cone.hasHit(tap1T)) &&
                ((Cone.hasHit(tap2T) && bodyT < tap2T) || !Cone.hasHit(tap2T))
            ) {
                winnerT = bodyT;
            } else if (
                Cone.hasHit(tap1T) &&
                ((Cone.hasHit(bodyT) && tap1T < bodyT) || !Cone.hasHit(bodyT)) &&
                ((Cone.hasHit(tap2T) && tap1T < tap2T) || !Cone.hasHit(tap2T))
            ) {
                winnerT = tap1T;
            } else if (Cone.hasHit(tap2T)) {
                winnerT = tap2T;
            }
        }

        if (!Cone.hasHit(winnerT)) {
            return false;
        }
        if (outHit !== null) {
            if (outHit.shouldStoreRay()) {
                outHit.setRay(inOutRay.withT(winnerT));
            } else {
                outHit.setHitDistance(winnerT);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.

    Java overload `doIntersectionFirstHit(Ray inOutRay)`.
    @param inOutRay
    @return true if given ray intersects current Cone
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit();
            if (this.doIntersectionFirstHit(inOutRay, hit)) {
                return hit.getRay();
            }
            return null;
        }

        if (outHit === null || !outHit.needsAnySurfaceData()) {
            return this.doIntersectionDistanceOnly(inOutRay, outHit);
        }

        const infoTap1 = new RayHit();
        const infoTap2 = new RayHit();
        const infoBody = new RayHit();
        let bodyHit: Ray | null;
        let tap1Hit: Ray | null;
        let tap2Hit: Ray | null;
        let winner: Ray | null = null;
        let winnerInfo: RayHit | null = null;

        if (this.topRadius < VSDK.EPSILON && this.bottomRadius > VSDK.EPSILON) {
            bodyHit = this.doIntersectionCone(inOutRay, this.bottomRadius, this.height, infoBody);
            tap1Hit = this.doIntersectionTap(inOutRay, this.bottomRadius, 0, infoTap1);
            if (
                (tap1Hit !== null && bodyHit === null) ||
                (tap1Hit !== null && bodyHit !== null && tap1Hit.getT() < bodyHit.getT())
            ) {
                infoTap1.normal = infoTap1.normal.multiply(-1);
                winner = inOutRay.withT(tap1Hit.getT());
                winnerInfo = infoTap1;
            } else if (bodyHit !== null) {
                winner = inOutRay.withT(bodyHit.getT());
                winnerInfo = infoBody;
            }
        } else if (VSDK.equals(this.bottomRadius, this.topRadius)) {
            let nearest = -1;
            bodyHit = this.doIntersectionCylinder(inOutRay, this.bottomRadius, this.height, infoBody);
            tap1Hit = this.doIntersectionTap(inOutRay, this.bottomRadius, 0, infoTap1);
            tap2Hit = this.doIntersectionTap(inOutRay, this.bottomRadius, this.height, infoTap2);

            if (
                bodyHit !== null &&
                ((tap1Hit !== null && bodyHit.getT() < tap1Hit.getT()) || tap1Hit === null) &&
                ((tap2Hit !== null && bodyHit.getT() < tap2Hit.getT()) || tap2Hit === null)
            ) {
                nearest = 1;
            } else if (
                tap1Hit !== null &&
                ((bodyHit !== null && tap1Hit.getT() < bodyHit.getT()) || bodyHit === null) &&
                ((tap2Hit !== null && tap1Hit.getT() < tap2Hit.getT()) || tap2Hit === null)
            ) {
                nearest = 3;
            } else if (tap2Hit !== null) {
                nearest = 2;
            }

            if (nearest === 1) {
                winner = inOutRay.withT(bodyHit!.getT());
                winnerInfo = infoBody;
            } else if (nearest === 2) {
                winner = inOutRay.withT(tap2Hit!.getT());
                winnerInfo = infoTap2;
            } else if (nearest === 3) {
                winner = inOutRay.withT(tap1Hit!.getT());
                winnerInfo = infoTap1;
            }
        }

        if (winner === null) {
            return false;
        }
        if (outHit !== null) {
            outHit.setRay(winner);
            outHit.point = new Vector3Dd(winnerInfo!.point);
            outHit.normal = new Vector3Dd(winnerInfo!.normal).normalized();
            outHit.tangent = new Vector3Dd(winnerInfo!.tangent);
            outHit.u = winnerInfo!.u;
            outHit.v = winnerInfo!.v;
            outHit.material = winnerInfo!.material;
            outHit.texture = winnerInfo!.texture;
            outHit.normalMap = winnerInfo!.normalMap;
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inRay
    @param inT
    @param outData
    */
    public override doExtraInformation(inRay: Ray, _inT: number, outData: RayHit): void {
        const hit = new RayHit();
        if (this.doIntersectionFirstHit(inRay.withT(Number.MAX_VALUE), hit)) {
            outData.clone(hit);
        }
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. The cone (or truncated cone, or cylinder)
    has its axis along Z, its base of radius `bottomRadius` at z = 0 and its
    top of radius `topRadius` at z = `height`. The distance to the side is
    measured perpendicular to the side, so the tolerance is the same for the
    side and for the caps.
    @param p point to classify, in the space of the cone
    @param distanceTolerance distance to the surface of the cone under which
    the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        if (this.height <= 0) {
            return Cone.OUTSIDE;
        }
        const z = p.z();
        const radialDistance = Math.sqrt(p.x() * p.x() + p.y() * p.y());
        const radiusAtZ = this.bottomRadius + (this.topRadius - this.bottomRadius) * (z / this.height);
        const radiusChange = this.bottomRadius - this.topRadius;
        const sideCosine = this.height / Math.sqrt(this.height * this.height + radiusChange * radiusChange);
        const sideDistance = (radialDistance - radiusAtZ) * sideCosine;

        if (z < -distanceTolerance || z > this.height + distanceTolerance || sideDistance > distanceTolerance) {
            return Cone.OUTSIDE;
        }
        if (z > distanceTolerance && z < this.height - distanceTolerance && sideDistance < -distanceTolerance) {
            return Cone.INSIDE;
        }
        return Cone.LIMIT;
    }

    // Finite solid bounds: |x|,|y| <= max radius and 0 <= z <= height.
    public override getBounds(result: GeometryBounds): void {
        const r = this.bottomRadius > this.topRadius ? this.bottomRadius : this.topRadius;
        if (
            !Double.isFinite(r) ||
            !Double.isFinite(this.height) ||
            this.height <= 0 ||
            this.bottomRadius < 0 ||
            this.topRadius < 0
        ) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(new Vector3Dd(-r, -r, 0), new Vector3Dd(r, r, this.height));
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): Float64Array {
        // TODO!
        const minmax = new Float64Array(6);

        const r = Math.max(this.bottomRadius, this.topRadius);

        minmax[0] = -r;
        minmax[1] = -r;
        minmax[2] = 0;
        minmax[3] = r;
        minmax[4] = r;
        minmax[5] = this.height;

        return minmax;
    }

    /**
    Java overloads `exportToPolyhedralBoundedSolid()` and
    `exportToPolyhedralBoundedSolid(int circumferenceDivisions, int heightDivisions)`,
    with the C++ `withCaps` parameter (default true).

    Boundary representation of the cone (a cylinder when both radii are
    equal), with its base on the plane z = 0 and its axis on +Z. The rings
    of vertices start at +X and go counterclockwise seen from +Z.

    With `withCaps` true (default) it is the closed solid: the lateral
    surface plus the base disk and, when the top radius is not zero, the top
    disk.

    With `withCaps` false it is a valid boundary representation of the
    lateral surface alone, an open curved surface without caps: a lamina
    ([MANT1988] 12.2, generalized from the planar circular lamina) with two
    coincident sides of opposite orientation, so it encloses no volume.
      - When both radii are not zero (cylinder or frustum) the lamina is
        built from a washer (`Tube.createWasherLamina`) whose inner ring is
        moved to z = height with the top radius, and both of its faces are
        split into the planar quadrilaterals between the two rings (a
        `lmekr` joins both loops of each face and `smef` splits it). The
        lamina has the topology of a torus (genus 1). Faces 1 and
        3 to `circumferenceDivisions` + 1 are the outer side (normals away
        from the axis); faces 2 and `circumferenceDivisions` + 2 to
        2 * `circumferenceDivisions` are the inner side.
      - When the top radius is zero (cone with apex) it is a circular lamina
        whose two faces are split into triangle fans, each one towards its
        own apex vertex (both at the same point), so the lamina has the
        topology of a sphere. Face 1 and faces 3 to
        `circumferenceDivisions` + 1 are the outer side; face 2 and the
        rest are the inner side.
    The open lamina has a single band of faces along the axis:
    `heightDivisions` is not used.

    @param circumferenceDivisions sides of the cone base (at least 3)
    @param heightDivisions vertical subdivisions of the closed solid (at
    least 1)
    @param withCaps true for the closed solid, false for the open lamina of
    the lateral surface
    @return a new boundary representation of the cone
    */
    public override exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(
        circumferenceDivisions: number,
        heightDivisions: number,
        withCaps?: boolean,
    ): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(
        circumferenceDivisions?: number,
        heightDivisions?: number,
        withCaps = true,
    ): PolyhedralBoundedSolid {
        if (circumferenceDivisions === undefined || heightDivisions === undefined) {
            return this.buildPolyhedralBoundedSolid(
                Cone.DEFAULT_CIRCUMFERENCE_DIVISIONS,
                Cone.DEFAULT_HEIGHT_DIVISIONS,
            );
        }
        const normalizedCircumferenceDivisions = Math.max(Cone.MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions);
        const normalizedHeightDivisions = Math.max(Cone.MIN_HEIGHT_DIVISIONS, heightDivisions);

        if (!withCaps) {
            return this.buildOpenLamina(normalizedCircumferenceDivisions);
        }
        if (
            normalizedCircumferenceDivisions === Cone.DEFAULT_CIRCUMFERENCE_DIVISIONS &&
            normalizedHeightDivisions === Cone.DEFAULT_HEIGHT_DIVISIONS
        ) {
            return this.exportToPolyhedralBoundedSolid();
        }

        return this.buildPolyhedralBoundedSolid(normalizedCircumferenceDivisions, normalizedHeightDivisions);
    }

    /**
    Current implementation of the cylinder follows the idea suggested on
    section [MANT1988].12.3.1 and program [MANT1988].12.4, where the
    cylinder is built upon a circular lamina base and an extrusion
    (translational sweep) operation. The cone case is done manually,
    */
    private static closeTopFaceToApex(solid: PolyhedralBoundedSolid, apexZ: number): void {
        Cone.closeFaceToApex(solid, 1, apexZ);
    }

    /**
    Splits face `faceId`, a single loop polygon, into a fan of triangles
    towards a new vertex at (0, 0, `apexZ`); the face keeps one of the
    triangles.
    */
    private static closeFaceToApex(solid: PolyhedralBoundedSolid, faceId: number, apexZ: number): void {
        Tube.splitFaceIntoFan(solid, faceId, new Vector3Dd(0.0, 0.0, apexZ));
    }

    private buildPolyhedralBoundedSolid(nsides: number, heightDivisions: number): PolyhedralBoundedSolid {
        let T: Matrix4x4d;
        let S: Matrix4x4d;
        let M: Matrix4x4d;

        const solid = PolyhedralBoundedSolidEulerConstructions.createCircularLamina(
            0.0,
            0.0,
            this.bottomRadius,
            0.0,
            nsides,
        );

        if (this.topRadius > VSDK.EPSILON && this.bottomRadius > VSDK.EPSILON) {
            let prevRadius = this.bottomRadius;
            const zStep = this.height / heightDivisions;
            let i: number;
            for (i = 1; i <= heightDivisions; i++) {
                const nextRadius = this.bottomRadius + (this.topRadius - this.bottomRadius) * (i / heightDivisions);
                const f = nextRadius / prevRadius;
                T = new Matrix4x4d();
                T = T.translation(0.0, 0.0, zStep);
                S = new Matrix4x4d();
                S = S.scale(f, f, 1.0);
                M = T.multiply(S);
                PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFacePlanar(
                    solid,
                    solid.findFace(1),
                    M,
                );
                prevRadius = nextRadius;
            }
        } else if (this.topRadius <= VSDK.EPSILON && this.bottomRadius > VSDK.EPSILON) {
            // Cone case, with optional vertical subdivisions.
            let prevRadius = this.bottomRadius;
            const zStep = this.height / heightDivisions;
            let i: number;
            for (i = 1; i < heightDivisions; i++) {
                const nextRadius = this.bottomRadius * (1.0 - i / heightDivisions);
                const f = nextRadius / prevRadius;
                T = new Matrix4x4d();
                T = T.translation(0.0, 0.0, zStep);
                S = new Matrix4x4d();
                S = S.scale(f, f, 1.0);
                M = T.multiply(S);
                PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFacePlanar(
                    solid,
                    solid.findFace(1),
                    M,
                );
                prevRadius = nextRadius;
            }
            Cone.closeTopFaceToApex(solid, this.height);
        }
        return solid;
    }

    /**
    Open lamina of the lateral surface (see `exportToPolyhedralBoundedSolid`
    with `withCaps` false).
    */
    private buildOpenLamina(nsides: number): PolyhedralBoundedSolid {
        let solid: PolyhedralBoundedSolid;
        let k: number;

        if (this.topRadius <= VSDK.EPSILON) {
            //- Cone with apex: both faces of a disk as fans, center lifted ---
            // The fans of faces 1 and 2 end at the vertices n+1 and n+2
            solid = Tube.createWasherLamina(0.0, this.bottomRadius, 0.0, nsides, true);
            solid.findVertex(nsides + 1)!.position = new Vector3Dd(0.0, 0.0, this.height);
            solid.findVertex(nsides + 2)!.position = new Vector3Dd(0.0, 0.0, this.height);
            PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
            return solid;
        }

        //- Washer split in quadrilaterals, internal circle lifted ----------
        // The external circle has the vertices 1..n at z = 0; the internal
        // vertex at the angle of the external vertex k+1 is
        // n+1+((n-k) mod n). Each one is placed above its external pair,
        // scaled to the top radius, so both have exactly the same longitude
        // and the quadrilaterals between them are planar.
        solid = Tube.createWasherLamina(0.5 * this.bottomRadius, this.bottomRadius, 0.0, nsides, true);
        const scale = this.topRadius / this.bottomRadius;
        for (k = 0; k < nsides; k++) {
            const bottom = solid.findVertex(k + 1)!;
            const top = solid.findVertex(nsides + 1 + ((nsides - k) % nsides))!;
            top.position = new Vector3Dd(bottom.position.x() * scale, bottom.position.y() * scale, this.height);
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }
}
