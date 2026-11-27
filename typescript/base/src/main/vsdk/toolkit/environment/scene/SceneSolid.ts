import { Double } from "../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../java/lang/Math.js";
import { Entity } from "../../common/Entity.js";
import { Matrix4x4d } from "../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../common/numerics/FloatingInterval.js";
import type { Geometry } from "../geometry/Geometry.js";
import { GeometryBounds } from "../geometry/element/GeometryBounds.js";
import { GeometryContainment } from "../geometry/element/GeometryContainment.js";
import { GeometryDistanceStatus } from "../geometry/element/GeometryDistanceStatus.js";
import { Ray } from "../geometry/element/Ray.js";
import type { RayCrossingQuery } from "../geometry/element/RayCrossingQuery.js";
import type { RayCrossingSink } from "../geometry/element/RayCrossingSink.js";
import { RayCrossingSupport } from "../geometry/element/RayCrossingSupport.js";
import type { RayHit } from "../geometry/element/RayHit.js";
import { RayInstanceStep } from "../geometry/element/RayInstanceStep.js";
import type { RaySolidClassification } from "../geometry/element/RaySolidClassification.js";
import { RaySolidQueryStatus } from "../geometry/element/RaySolidQueryStatus.js";
import { AffineEllipsoidDistance } from "../geometry/volume/AffineEllipsoidDistance.js";
import { AffineEllipsoidDistanceBounds } from "../geometry/volume/AffineEllipsoidDistanceBounds.js";
import { AffineEllipsoidMinimumDistance } from "../geometry/volume/AffineEllipsoidMinimumDistance.js";
import { Solid } from "../geometry/volume/Solid.js";
import { Sphere } from "../geometry/volume/Sphere.js";
import type { Material } from "../material/Material.js";
import { AffinePlacementStatus } from "./AffinePlacementStatus.js";
import { AffineToleranceMap } from "./AffineToleranceMap.js";
import { InstanceRayCrossingBuffer } from "./InstanceRayCrossingBuffer.js";

/**
The scene element contract shared by every application's bodies: a geometry
reference, an authoritative affine placement (column vectors, geometry to
parent) with its inverse, and the queries of the placed solid. The typed
queries are computed here, once, from the geometry and
`queryAffinePlacement`; implementations add their own placement storage,
appearance and first hit paths, and override a typed query only when a scene
policy changes the solid they denote (CSG, clipping regions, inversion).
Bounds, box classification and solid equivalence apply only to bodies that
opt in through `denotesPlacedGeometry`.

`SimpleBody` is the VITRAL implementation; applications may provide their own
(PovRay's `PovrayBody`) referencing the same `Geometry` types.

Output arguments follow the port conventions: matrices, vectors and primitive
values are returned through one-element arrays.
*/
export abstract class SceneSolid extends Entity {
    protected geometry: Geometry | null;
    /// False when the geometry is shared (see `setGeometryReference`)
    private ownsGeometry: boolean;
    private geometryIsSphere: boolean;
    private name: string;
    private modificationVersion: number;

    public constructor() {
        super();
        this.geometry = null;
        this.ownsGeometry = true;
        this.geometryIsSphere = false;
        this.name = "";
        this.modificationVersion = 0;
    }

    /**
    Whether the solid of this body is exactly its geometry under its
    placement, with no scene policy that changes membership. Only such bodies
    report solid bounds, certified box classification and an equivalence
    geometry. False by default, so unknown implementations do not opt in.
    */
    protected denotesPlacedGeometry(): boolean {
        return false;
    }

    // Whether a CSG traversal may descend into this body's geometry as its
    // solid: exactly when the body denotes its placed geometry.
    public isStructuralTraversalSafe(): boolean {
        return this.denotesPlacedGeometry();
    }

    /**
    The intrinsic solid, and whether the body denotes its complement, when
    this body's solid is that solid under its placement with no scene policy
    changing membership. By default the geometry itself, only for bodies that
    `denotesPlacedGeometry`. Bodies that wrap or invert their geometry return
    the wrapped solid and the inversion. Box classification, solid
    equivalence and the boundary distances are computed from it.
    @return false when no such solid exists
    */
    protected queryDenotedSolid(intrinsic: (Geometry | null)[], inverted: boolean[]): boolean {
        intrinsic[0] = null;
        inverted[0] = false;
        if (!this.denotesPlacedGeometry() || this.geometry === null) return false;
        intrinsic[0] = this.geometry;
        return true;
    }

    private static orientSide(side: GeometryContainment, inverted: boolean): GeometryContainment {
        if (inverted && side === GeometryContainment.Inside) return GeometryContainment.Outside;
        if (inverted && side === GeometryContainment.Outside) return GeometryContainment.Inside;
        return side;
    }

    private queryDenotedSphere(
        sphere: (Sphere | null)[],
        inverted: boolean[],
        forward: Matrix4x4d[],
        inverse: Matrix4x4d[],
    ): GeometryDistanceStatus {
        const intrinsic: (Geometry | null)[] = [null];
        if (!this.queryDenotedSolid(intrinsic, inverted)) return GeometryDistanceStatus.Unsupported;
        sphere[0] = intrinsic[0] instanceof Sphere ? intrinsic[0] : null;
        if (sphere[0] === null) return GeometryDistanceStatus.Unsupported;
        const status = this.queryAffinePlacement(forward, inverse);
        if (status === AffinePlacementStatus.Invalid) return GeometryDistanceStatus.InvalidInput;
        if (status === AffinePlacementStatus.Unsupported) return GeometryDistanceStatus.Unsupported;
        if (status === AffinePlacementStatus.Indeterminate) return GeometryDistanceStatus.Indeterminate;
        return GeometryDistanceStatus.Valid;
    }

    protected hasSphereGeometry(): boolean {
        return this.geometryIsSphere;
    }

    /// The material reported with this body's ray crossings, or null
    public getSurfaceMaterial(): Material | null {
        return null;
    }

    // Result is in the parent coordinate system of this body.
    public getWorldBounds(result: GeometryBounds): void {
        const shape = this.getGeometry();
        if (shape === null) {
            result.setEmpty();
            return;
        }
        const local = new GeometryBounds();
        shape.getBounds(local);
        result.assign(local.transformed(this.getAffinePlacement()));
    }

    // Conservative world bounds of the solid this body denotes. Only bodies
    // that denote their placed geometry, over Solid geometry, qualify; others
    // report Unsupported, so bounds are never mistaken for volumetric membership.
    public getSolidWorldBounds(result: GeometryBounds): void {
        if (!this.denotesPlacedGeometry() || !(this.getGeometry() instanceof Solid)) {
            result.setUnsupported();
            return;
        }
        this.getWorldBounds(result);
    }

    // Certified classification of a closed world-space axis-aligned box:
    // Inside/Outside only when the whole box lies strictly in the interior or
    // exterior of this body's solid; never Boundary.
    public classifyWorldBoxCertified(box: GeometryBounds): GeometryContainment {
        const intrinsic: (Geometry | null)[] = [null];
        const inverted = [false];
        if (!this.queryDenotedSolid(intrinsic, inverted)) return GeometryContainment.Unsupported;
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const placement = this.queryAffinePlacement(forward, inverse);
        if (placement === AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
        if (placement === AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
        if (placement === AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
        return SceneSolid.classifyPlacedBoxCertified(intrinsic[0]!, inverted[0]!, inverse[0]!, box);
    }

    // The single certified world-space point classification for an intrinsic
    // solid under a validated affine pair. Spheres use the ellipsoid distance
    // certificates; other solids map the tolerance ball to local inner and outer
    // balls. An inverted binding swaps Inside and Outside. Scene specializations
    // and CSG operands reuse this routine rather than reimplementing a certificate.
    public static classifyPlacedBoxCertified(
        geometry: Geometry,
        inverted: boolean,
        inverse: Matrix4x4d,
        box: GeometryBounds,
    ): GeometryContainment {
        if (box.getKind() === GeometryBounds.Kind.Unsupported) return GeometryContainment.Unsupported;
        if (box.getKind() !== GeometryBounds.Kind.Finite) return GeometryContainment.Indeterminate;
        // The conservative local bounds of the box's preimage, as centre and
        // outward-rounded half widths.
        const local = box.transformed(inverse);
        if (local.getKind() !== GeometryBounds.Kind.Finite) return GeometryContainment.Indeterminate;
        const lo = [local.getMinimum().x(), local.getMinimum().y(), local.getMinimum().z()];
        const hi = [local.getMaximum().x(), local.getMaximum().y(), local.getMaximum().z()];
        const centre = [0, 0, 0],
            half = [0, 0, 0];
        for (let i = 0; i < 3; ++i) {
            centre[i] = lo[i]! * 0.5 + hi[i]! * 0.5;
            const below = FloatingInterval.exact(centre[i]!).subtract(FloatingInterval.exact(lo[i]!));
            const above = FloatingInterval.exact(hi[i]!).subtract(FloatingInterval.exact(centre[i]!));
            if (!Double.isFinite(centre[i]!) || !below.isValid() || !above.isValid())
                return GeometryContainment.Indeterminate;
            half[i] = JavaMath.max(JavaMath.max(below.getUpper(), above.getUpper()), 0.0);
        }
        const result = geometry.classifyBoxCertified(
            new Vector3Dd(centre[0]!, centre[1]!, centre[2]!),
            new Vector3Dd(half[0]!, half[1]!, half[2]!),
        );
        if (inverted && result === GeometryContainment.Inside) return GeometryContainment.Outside;
        if (inverted && result === GeometryContainment.Outside) return GeometryContainment.Inside;
        return result;
    }

    public emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        if (this.geometry === null) return RayCrossingSupport.Unsupported;
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const status = this.queryAffinePlacement(forward, inverse);
        if (status === AffinePlacementStatus.Unsupported) return RayCrossingSupport.Unsupported;
        if (status === AffinePlacementStatus.Invalid) return RayCrossingSupport.InvalidInput;
        if (status === AffinePlacementStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
        const origin = inverse[0]!.multiply(ray.getOrigin());
        const direction = inverse[0]!.withoutTranslation().multiply(ray.getDirection());
        if (
            !Double.isFinite(origin.x()) ||
            !Double.isFinite(origin.y()) ||
            !Double.isFinite(origin.z()) ||
            !Double.isFinite(direction.x()) ||
            !Double.isFinite(direction.y()) ||
            !Double.isFinite(direction.z())
        )
            return RayCrossingSupport.Indeterminate;
        const local = Ray.fromAffine(origin, direction, ray.getT());
        const buffer = new InstanceRayCrossingBuffer(
            new RayInstanceStep(this, forward[0]!, inverse[0]!),
            this,
            this.getSurfaceMaterial(),
        );
        const result = this.geometry.emitRayCrossings(local, query, buffer);
        if (result !== RayCrossingSupport.Supported) return result;
        if (!buffer.isValid()) return RayCrossingSupport.Indeterminate;
        buffer.deliver(sink);
        return RayCrossingSupport.Supported;
    }

    public classifyRay(ray: Ray, query: RayCrossingQuery, result: RaySolidClassification): RaySolidQueryStatus {
        result.clear();
        if (this.geometry === null) return RaySolidQueryStatus.Unsupported;
        const childToParent = [new Matrix4x4d()],
            parentToChild = [new Matrix4x4d()];
        const placement = this.queryAffinePlacement(childToParent, parentToChild);
        if (placement === AffinePlacementStatus.Unsupported) return RaySolidQueryStatus.Unsupported;
        if (placement === AffinePlacementStatus.Invalid) return RaySolidQueryStatus.InvalidInput;
        if (placement === AffinePlacementStatus.Indeterminate) return RaySolidQueryStatus.Indeterminate;
        const origin = ray.getOrigin();
        const direction = ray.getDirection();
        if (
            !Double.isFinite(origin.x()) ||
            !Double.isFinite(origin.y()) ||
            !Double.isFinite(origin.z()) ||
            !Double.isFinite(direction.x()) ||
            !Double.isFinite(direction.y()) ||
            !Double.isFinite(direction.z())
        )
            return RaySolidQueryStatus.InvalidInput;
        for (let row = 0; row < 4; row++) {
            for (let column = 0; column < 4; column++) {
                if (
                    !Double.isFinite(childToParent[0]!.get(row, column)) ||
                    !Double.isFinite(parentToChild[0]!.get(row, column))
                )
                    return RaySolidQueryStatus.Indeterminate;
            }
        }
        const localOrigin = parentToChild[0]!.multiply(origin);
        const localDirection = parentToChild[0]!.withoutTranslation().multiply(direction);
        if (
            !Double.isFinite(localOrigin.x()) ||
            !Double.isFinite(localOrigin.y()) ||
            !Double.isFinite(localOrigin.z()) ||
            !Double.isFinite(localDirection.x()) ||
            !Double.isFinite(localDirection.y()) ||
            !Double.isFinite(localDirection.z())
        )
            return RaySolidQueryStatus.Indeterminate;
        const localRay = Ray.fromAffine(localOrigin, localDirection, ray.getT());
        const status = this.geometry.classifyRay(localRay, query, result);
        if (status !== RaySolidQueryStatus.Supported) {
            result.clear();
            return status;
        }
        const step = new RayInstanceStep(this, childToParent[0]!, parentToChild[0]!);
        for (let i = 0; i < result.boundaries.size(); i++) {
            if (!result.boundaries.get(i).appendInstance(step)) {
                result.clear();
                return RaySolidQueryStatus.Indeterminate;
            }
        }
        return status;
    }

    // Certified minimum-distance enclosure no wider than maxWidth. Failure
    // leaves all outputs untouched; currently supported for placed spheres.
    public encloseMinimumBoundaryDistance(
        point: Vector3Dd,
        maxWidth: number,
        side: GeometryContainment[],
        lower: number[],
        upper: number[],
    ): GeometryDistanceStatus {
        const sphere: (Sphere | null)[] = [null];
        const inverted = [false];
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const availability = this.queryDenotedSphere(sphere, inverted, forward, inverse);
        if (availability !== GeometryDistanceStatus.Valid) return availability;
        const candidateSide = [GeometryContainment.Indeterminate];
        const candidateLower = [0],
            candidateUpper = [0];
        const status = AffineEllipsoidMinimumDistance.query(
            point,
            sphere[0]!.getRadius(),
            forward[0]!,
            inverse[0]!,
            maxWidth,
            candidateSide,
            candidateLower,
            candidateUpper,
        );
        if (status !== GeometryDistanceStatus.Valid) return status;
        side[0] = SceneSolid.orientSide(candidateSide[0]!, inverted[0]!);
        lower[0] = candidateLower[0]!;
        upper[0] = candidateUpper[0]!;
        return GeometryDistanceStatus.Valid;
    }

    // Certified enclosure for a placed sphere and a nonzero local direction.
    // The lower bound may be zero; all outputs are unchanged on failure.
    public encloseBoundaryDistance(
        point: Vector3Dd,
        localDirection: Vector3Dd,
        side: GeometryContainment[],
        lower: number[],
        upper: number[],
    ): GeometryDistanceStatus {
        const sphere: (Sphere | null)[] = [null];
        const inverted = [false];
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const availability = this.queryDenotedSphere(sphere, inverted, forward, inverse);
        if (availability !== GeometryDistanceStatus.Valid) return availability;
        const candidateSide = [GeometryContainment.Indeterminate];
        const candidateLower = [0],
            candidateUpper = [0];
        const status = AffineEllipsoidDistanceBounds.query(
            point,
            sphere[0]!.getRadius(),
            forward[0]!,
            localDirection,
            candidateSide,
            candidateLower,
            candidateUpper,
        );
        if (status !== GeometryDistanceStatus.Valid) return status;
        side[0] = SceneSolid.orientSide(candidateSide[0]!, inverted[0]!);
        lower[0] = candidateLower[0]!;
        upper[0] = candidateUpper[0]!;
        return GeometryDistanceStatus.Valid;
    }

    // Borrowed core solid only when scene-specific policies preserve it exactly.
    // Unknown subclasses must opt in explicitly; null means not comparable.
    public getSolidEquivalenceGeometry(): Geometry | null {
        const intrinsic: (Geometry | null)[] = [null];
        const inverted = [false];
        return this.queryDenotedSolid(intrinsic, inverted) && !inverted[0] ? intrinsic[0]! : null;
    }

    // Only decides when the enclosure proves the result; otherwise Indeterminate.
    public classifyPointCertified(point: Vector3Dd, tolerance: number): GeometryContainment {
        if (
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) ||
            !Double.isFinite(tolerance) ||
            tolerance < 0
        )
            return GeometryContainment.InvalidInput;
        if (this.geometry === null) return GeometryContainment.Unsupported;
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const placement = this.queryAffinePlacement(forward, inverse);
        if (placement === AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
        if (placement === AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
        if (placement === AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
        return SceneSolid.classifyPlacedPointCertified(
            this.geometry,
            false,
            forward[0]!,
            inverse[0]!,
            point,
            tolerance,
        );
    }

    public static classifyPlacedPointCertified(
        geometry: Geometry,
        inverted: boolean,
        forward: Matrix4x4d,
        inverse: Matrix4x4d,
        point: Vector3Dd,
        tolerance: number,
    ): GeometryContainment {
        if (
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) ||
            !Double.isFinite(tolerance) ||
            tolerance < 0
        )
            return GeometryContainment.InvalidInput;
        const sphere = geometry instanceof Sphere ? geometry : null;
        const result = sphere
            ? SceneSolid.classifyPlacedSphereCertified(sphere.getRadius(), forward, inverse, point, tolerance, 128)
            : SceneSolid.classifyPlacedSolidCertified(geometry, forward, point, tolerance);
        if (inverted && result === GeometryContainment.Inside) return GeometryContainment.Outside;
        if (inverted && result === GeometryContainment.Outside) return GeometryContainment.Inside;
        return result;
    }

    private static classifyPlacedSolidCertified(
        geometry: Geometry,
        forward: Matrix4x4d,
        point: Vector3Dd,
        tolerance: number,
    ): GeometryContainment {
        let identity = true;
        for (let row = 0; row < 4; ++row)
            for (let column = 0; column < 4; ++column)
                if (forward.get(row, column) !== (row === column ? 1.0 : 0.0)) identity = false;
        if (identity) return geometry.classifyPointCertified(point, tolerance);
        const localOut = [new Vector3Dd()],
            halfWidthsOut = [new Vector3Dd()];
        const outerOut = [0],
            innerOut = [-1];
        const mapped = AffineToleranceMap.map(forward, point, tolerance, localOut, outerOut, innerOut, halfWidthsOut);
        if (mapped === AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
        if (mapped === AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
        if (mapped !== AffinePlacementStatus.Valid) return GeometryContainment.Indeterminate;
        const local = localOut[0]!,
            halfWidths = halfWidthsOut[0]!,
            outer = outerOut[0]!,
            inner = innerOut[0]!;
        // The axis-aligned local box also contains the preimage and is tighter
        // under anisotropic placement; it can only prove Inside or Outside.
        const region = geometry.classifyBoxCertified(local, halfWidths);
        if (
            region === GeometryContainment.Inside ||
            region === GeometryContainment.Outside ||
            region === GeometryContainment.InvalidInput
        )
            return region;
        // The outer local ball contains the preimage of the world ball, so a
        // definite side for it is a definite world side. A zero outer radius means
        // the preimage is exactly the local point.
        const result = geometry.classifyPointCertified(local, outer);
        if (
            result === GeometryContainment.Inside ||
            result === GeometryContainment.Outside ||
            result === GeometryContainment.InvalidInput ||
            result === GeometryContainment.Unsupported
        )
            return result;
        if (result === GeometryContainment.Boundary && outer === 0) return result;
        // A boundary inside the inner local ball maps inside the world ball.
        if (inner > 0) {
            const near = geometry.classifyPointCertified(local, inner);
            if (
                near === GeometryContainment.Boundary ||
                near === GeometryContainment.InvalidInput ||
                near === GeometryContainment.Unsupported
            )
                return near;
        }
        if (!(tolerance > 0)) return GeometryContainment.Indeterminate;
        // Under anisotropic placement the inner ball is small. Two world points of
        // the (convex) tolerance ball with certified opposite membership prove a
        // boundary point on the segment joining them.
        const centre = SceneSolid.classifyPlacedSolidCertified(geometry, forward, point, 0);
        const centreSign = centre === GeometryContainment.Inside ? -1 : centre === GeometryContainment.Outside ? 1 : 0;
        let foundInside = centreSign < 0,
            foundOutside = centreSign > 0;
        const step = tolerance * (1.0 - 1.0 / 1048576.0);
        const diagonal = (step / JavaMath.sqrt(3.0)) * (1.0 - 1.0 / 1048576.0);
        for (let direction = 0; direction < 14; ++direction) {
            let offset: Vector3Dd;
            if (direction < 6) {
                const sign = direction % 2 === 0 ? 1.0 : -1.0;
                const axis = Math.trunc(direction / 2);
                offset = new Vector3Dd(
                    axis === 0 ? sign * step : 0,
                    axis === 1 ? sign * step : 0,
                    axis === 2 ? sign * step : 0,
                );
            } else {
                const corner = direction - 6;
                offset = new Vector3Dd(
                    corner & 1 ? -diagonal : diagonal,
                    corner & 2 ? -diagonal : diagonal,
                    corner & 4 ? -diagonal : diagonal,
                );
            }
            const sample = point.add(offset);
            let squared = FloatingInterval.exact(0);
            squared = squared.add(
                FloatingInterval.exact(sample.x()).subtract(FloatingInterval.exact(point.x())).square(),
            );
            squared = squared.add(
                FloatingInterval.exact(sample.y()).subtract(FloatingInterval.exact(point.y())).square(),
            );
            squared = squared.add(
                FloatingInterval.exact(sample.z()).subtract(FloatingInterval.exact(point.z())).square(),
            );
            const distance = squared.squareRoot();
            if (!distance.isValid() || distance.getUpper() > tolerance) continue;
            const side = SceneSolid.classifyPlacedSolidCertified(geometry, forward, sample, 0);
            if (side === GeometryContainment.Inside) foundInside = true;
            else if (side === GeometryContainment.Outside) foundOutside = true;
            else if (side === GeometryContainment.Boundary) return GeometryContainment.Boundary;
            if (foundInside && foundOutside) return GeometryContainment.Boundary;
        }
        return GeometryContainment.Indeterminate;
    }

    private static classifyPlacedSphereCertified(
        radius: number,
        forward: Matrix4x4d,
        inverse: Matrix4x4d,
        point: Vector3Dd,
        tolerance: number,
        iterationLimit: number,
    ): GeometryContainment {
        let seed = inverse.multiply(point);
        if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) || !Double.isFinite(seed.z()))
            return GeometryContainment.Indeterminate;
        const center =
            (point.x() === forward.get(0, 3) && point.y() === forward.get(1, 3) && point.z() === forward.get(2, 3)) ||
            (seed.x() === 0 && seed.y() === 0 && seed.z() === 0);
        const axes = [new Vector3Dd(1, 0, 0), new Vector3Dd(0, 1, 0), new Vector3Dd(0, 0, 1)];
        let forwardNormSquared = 0;
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) forwardNormSquared += forward.get(i, j) * forward.get(i, j);
        const lipschitz = radius * radius * forwardNormSquared;
        const seedCount = center ? 3 : 1;
        for (let candidate = 0; candidate < seedCount; ++candidate) {
            if (center) seed = axes[candidate]!;
            for (let iteration = 0; iteration < iterationLimit; ++iteration) {
                if (iteration === 0 || iteration % 4 === 0 || iteration + 1 === iterationLimit) {
                    const side = [GeometryContainment.Indeterminate];
                    const lower = [0],
                        upper = [0];
                    const status = AffineEllipsoidDistanceBounds.query(
                        point,
                        radius,
                        forward,
                        seed,
                        side,
                        lower,
                        upper,
                        iteration === 0 && candidate === 0,
                    );
                    if (status === GeometryDistanceStatus.InvalidInput) return GeometryContainment.InvalidInput;
                    if (status === GeometryDistanceStatus.Unsupported) return GeometryContainment.Unsupported;
                    if (status !== GeometryDistanceStatus.Valid) return GeometryContainment.Indeterminate;
                    if (upper[0]! <= tolerance) return GeometryContainment.Boundary;
                    if (lower[0]! > tolerance) return side[0]!;
                }
                if (!(lipschitz > 0) || !Double.isFinite(lipschitz)) break;
                const seedLength = seed.length();
                if (!(seedLength > 0) || !Double.isFinite(seedLength)) break;
                const unit = seed.multiply(1.0 / seedLength);
                const delta = forward.multiply(unit.multiply(radius)).subtract(point);
                const components = [delta.x(), delta.y(), delta.z()];
                const gradient = [0, 0, 0];
                for (let i = 0; i < 3; ++i)
                    for (let j = 0; j < 3; ++j)
                        gradient[i] = gradient[i]! + radius * forward.get(j, i) * components[j]!;
                const g = new Vector3Dd(gradient[0]!, gradient[1]!, gradient[2]!);
                const tangent = g.subtract(unit.multiply(g.dotProduct(unit)));
                if (tangent.dotProduct(tangent) === 0) break;
                seed = unit.subtract(tangent.multiply(1.0 / lipschitz));
                if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) || !Double.isFinite(seed.z())) break;
            }
        }
        // If surface-directed search cannot decide, use a separately certified
        // minimum-distance enclosure. The numerical estimate only proposes its
        // witness; interval bounds, never the estimate, determine this result.
        const requestedWidth = tolerance * 1e-3;
        if (requestedWidth > 0 && Double.isFinite(requestedWidth)) {
            const side = [GeometryContainment.Indeterminate];
            const lower = [0],
                upper = [0];
            const status = AffineEllipsoidMinimumDistance.query(
                point,
                radius,
                forward,
                inverse,
                requestedWidth,
                side,
                lower,
                upper,
            );
            if (status === GeometryDistanceStatus.InvalidInput) return GeometryContainment.InvalidInput;
            if (status === GeometryDistanceStatus.Unsupported) return GeometryContainment.Unsupported;
            if (status === GeometryDistanceStatus.Valid) {
                if (upper[0]! <= tolerance) return GeometryContainment.Boundary;
                if (lower[0]! > tolerance) return side[0]!;
            }
        }
        return GeometryContainment.Indeterminate;
    }

    // Numerical estimate for placed spheres; estimatedError is not certified.
    // On failure both output arguments remain unchanged.
    public estimateBoundaryDistance(
        point: Vector3Dd,
        distance: number[],
        estimatedError: number[],
    ): GeometryDistanceStatus {
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) || !Double.isFinite(point.z()))
            return GeometryDistanceStatus.InvalidInput;
        const sphere: (Sphere | null)[] = [null];
        const inverted = [false];
        const forward = [new Matrix4x4d()],
            inverse = [new Matrix4x4d()];
        const availability = this.queryDenotedSphere(sphere, inverted, forward, inverse);
        if (availability !== GeometryDistanceStatus.Valid) return availability;
        return AffineEllipsoidDistance.query(point, sphere[0]!.getRadius(), forward[0]!, distance, estimatedError);
    }

    // Zero tolerance classifies the mapped local point. Positive world-space
    // tolerance uses the certified classification below.
    public classifyPoint(point: Vector3Dd, tolerance: number): GeometryContainment {
        if (this.geometry === null) return GeometryContainment.Unsupported;
        if (
            !Double.isFinite(tolerance) ||
            tolerance < 0 ||
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())
        )
            return GeometryContainment.InvalidInput;
        if (tolerance === 0) {
            const forward = [new Matrix4x4d()],
                inverse = [new Matrix4x4d()];
            const status = this.queryAffinePlacement(forward, inverse);
            if (status === AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
            if (status === AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
            if (status === AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
            const local = inverse[0]!.multiply(point);
            if (!Double.isFinite(local.x()) || !Double.isFinite(local.y()) || !Double.isFinite(local.z()))
                return GeometryContainment.Indeterminate;
            return this.geometry.classifyPoint(local, 0);
        }
        return this.classifyPointCertified(point, tolerance);
    }

    protected markModified(): void {
        this.modificationVersion++;
    }

    public getName(): string {
        return this.name;
    }

    public getModificationVersion(): number {
        return this.modificationVersion;
    }

    public setName(n: string): void {
        this.name = n;
        this.markModified();
    }

    public getGeometry(): Geometry | null {
        return this.geometry;
    }

    /**
    Sets the geometry, taking its ownership (it is deleted with this body or
    when replaced).
    @param g new geometry, or null
    */
    public setGeometry(g: Geometry | null): void {
        this.geometry = g;
        this.ownsGeometry = true;
        this.geometryIsSphere = this.geometry instanceof Sphere;
        this.markModified();
    }

    /**
    Sets a geometry shared with other owners (i.e. a primitive instanced by
    several bodies): this body does not delete it.
    @param g shared geometry, or null
    */
    public setGeometryReference(g: Geometry | null): void {
        this.geometry = g;
        this.ownsGeometry = false;
        this.geometryIsSphere = this.geometry instanceof Sphere;
        this.markModified();
    }

    // Column-vector affine geometry-to-parent placement, including all layers.
    public abstract getAffinePlacement(): Matrix4x4d;
    // On failure the previous placement remains unchanged.
    public abstract setAffinePlacement(forward: Matrix4x4d, inverse: Matrix4x4d): AffinePlacementStatus;
    // Reset to identity.
    public abstract resetAffinePlacement(): void;
    public abstract queryAffinePlacement(forward: Matrix4x4d[], inverse: Matrix4x4d[]): AffinePlacementStatus;

    public abstract doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit | null): boolean;
    public abstract doExtraInformation(inRay: Ray, inT: number, outData: RayHit | null): void;
}
