//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [ROTH1982] Roth, Scott D. "Ray Casting for Modeling Solids", Computer  =
//=     Graphics and Image Processing 18, 109-144, 1982.                    =

import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { VSDK } from "../../../common/VSDK.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector2Dd } from "../../../common/linealAlgebra/Vector2Dd.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Geometry } from "../Geometry.js";
import type { GeometryBounds } from "../element/GeometryBounds.js";
import { GeometryContainment } from "../element/GeometryContainment.js";
import type { Ray } from "../element/Ray.js";
import { RayBoundary } from "../element/RayBoundary.js";
import { RayCrossing } from "../element/RayCrossing.js";
import { RayCrossingKind } from "../element/RayCrossingKind.js";
import { RayCrossingQuery } from "../element/RayCrossingQuery.js";
import type { RayCrossingSink } from "../element/RayCrossingSink.js";
import { RayCrossingSupport } from "../element/RayCrossingSupport.js";
import { RayFirstHitStatus } from "../element/RayFirstHitStatus.js";
import { RayHit } from "../element/RayHit.js";
import { RaySolidBoundary } from "../element/RaySolidBoundary.js";
import type { RaySolidClassification } from "../element/RaySolidClassification.js";
import { RaySolidQueryStatus } from "../element/RaySolidQueryStatus.js";
import { PolyhedralBoundedSolid } from "./polyhedralBoundedSolid/PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidEulerConstructions } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.js";
import { PolyhedralBoundedSolidEulerOperators } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.js";
import { Solid } from "./Solid.js";

/**
Interval [start, end] of the parameter of a line, whose ends are on the
given surfaces of the prism (`SURFACE_NONE` for an unbounded end).

C++ private nested class `Prism::Interval` (a value type: copies are made
with `copy()` where C++ copies by value).
*/
class PrismInterval {
    public start: number;
    public end: number;
    public startSurface: number;
    public endSurface: number;

    public constructor(start = 0, startSurface = -1, end = 0, endSurface = -1) {
        this.start = start;
        this.end = end;
        this.startSurface = startSurface;
        this.endSurface = endSurface;
    }

    public copy(): PrismInterval {
        return new PrismInterval(this.start, this.startSurface, this.end, this.endSurface);
    }
}

/**
A crossing of a line with an edge of the base, on the plane z = 0.

C++ private nested class `Prism::EdgeCrossing`.
*/
class PrismEdgeCrossing {
    public lineParameter: number;
    public edge: number;

    public constructor(t = 0, edge = 0) {
        this.lineParameter = t;
        this.edge = edge;
    }
}

/**
A right prism: a simple polygon (the base) on the plane z = 0, swept
`height` units along +Z. The base may be concave; it may not cross itself.
Its vertices are kept counterclockwise seen from +Z (a clockwise polygon is
reversed when it is given), so the walls follow the edges of the base.

The prism is a parametric solid: rays are intersected and classified
against its definition (no boundary representation is built), so it can be
an operand of the constructive solid geometry evaluated with the ray
classification of [ROTH1982] (`classifyRay`). The interior along a line is
computed as the intersection of the slab 0 <= z <= height with the parts of
the line, projected on the plane z = 0, inside the base; for a concave base
there can be several intervals.

The surfaces of the prism are numbered (`RayBoundary.surfaceIndex`):
`SURFACE_BOTTOM` (z = 0), `SURFACE_TOP` (z = height) and, for each edge i
of the base (from vertex i to vertex i+1), the wall `SURFACE_FIRST_WALL` + i.

For generic editors (as the scene editor) it exposes the number of sides of
the base, its circumradius (the largest distance from the Z axis to a vertex
of the base) and its height. Changing the number of sides replaces the base
by a regular polygon with the same circumradius; changing the circumradius
scales the base about the Z axis.
*/
export class Prism extends Solid {
    public static readonly SURFACE_NONE = -1;
    public static readonly SURFACE_BOTTOM = 0;
    public static readonly SURFACE_TOP = 1;
    public static readonly SURFACE_FIRST_WALL = 2;

    private readonly base = new ArrayList<Vector2Dd>();
    private height: number;
    /// Cached from the base: true when it is a simple polygon of non-zero area
    private validBase = false;
    private minX = 0;
    private minY = 0;
    private maxX = 0;
    private maxY = 0;

    //= Construction and parameters ======================================

    /**
    C++ overloads:
      - `Prism(int numberOfSides, double circumradius, double height)`:
        regular prism, the base is a regular polygon of `numberOfSides`
        vertices at distance `circumradius` from the Z axis, the first one
        on +X.
      - `Prism(const java::ArrayList<Vector2Dd>& base, double height)`:
        `base` holds the vertices of a simple polygon on the plane z = 0, in
        either orientation (it is stored counterclockwise), and `height` is
        the length of the sweep along +Z.
    */
    public constructor(numberOfSides: number, circumradius: number, height: number);
    public constructor(base: ArrayList<Vector2Dd>, height: number);
    public constructor(a: number | ArrayList<Vector2Dd>, b: number, c?: number) {
        super();
        if (a instanceof ArrayList) {
            this.height = b;
            this.setBase(a);
        } else {
            this.height = c!;
            const n = a < 3 ? 3 : a;
            const polygon = new ArrayList<Vector2Dd>();
            for (let i = 0; i < n; i++) {
                const angle = (2.0 * JavaMath.PI * i) / n;
                polygon.add(new Vector2Dd(b * JavaMath.cos(angle), b * JavaMath.sin(angle)));
            }
            this.setBase(polygon);
        }
        this.addControlSpecifications();
    }

    private addControlSpecifications(): void {
        this.getControlSpecifications().push("int;numberOfSides;[3, INFINITE)");
        this.getControlSpecifications().push("double;circumradius;(0, INFINITE)");
        this.getControlSpecifications().push("double;height;(0, INFINITE)");
    }

    public override copyGeometry(): Geometry | null {
        return new Prism(this.base, this.height);
    }

    public getBase(): ArrayList<Vector2Dd> {
        return this.base;
    }

    public setBase(value: ArrayList<Vector2Dd>): void {
        // Shoelace formula: a clockwise polygon has negative area
        let twiceArea = 0.0;
        const n = value.size();
        for (let i = 0; i < n; i++) {
            const a = value.get(i);
            const b = value.get((i + 1) % n);
            twiceArea += a.x * b.y - b.x * a.y;
        }
        // C++ copies the vertices by value: the source may be this.base
        const copies: Vector2Dd[] = [];
        for (let i = 0; i < n; i++) {
            copies.push(new Vector2Dd(value.get(i)));
        }
        this.base.clear();
        if (twiceArea < 0.0) {
            for (let i = n - 1; i >= 0; i--) {
                this.base.add(copies[i]!);
            }
        } else {
            for (let i = 0; i < n; i++) {
                this.base.add(copies[i]!);
            }
        }
        this.updateBaseCache();
    }

    public getHeight(): number {
        return this.height;
    }

    public setHeight(value: number): void {
        this.height = value;
    }

    public getNumberOfSides(): number {
        return this.base.size();
    }

    public setNumberOfSides(value: number): void {
        const n = value < 3 ? 3 : value;
        let r = this.getCircumradius();
        if (!(r > 0.0) || !Double.isFinite(r)) {
            r = 1.0;
        }
        const polygon = new ArrayList<Vector2Dd>();
        for (let i = 0; i < n; i++) {
            const angle = (2.0 * JavaMath.PI * i) / n;
            polygon.add(new Vector2Dd(r * JavaMath.cos(angle), r * JavaMath.sin(angle)));
        }
        this.setBase(polygon);
    }

    /**
    C++ `Vector2Dd::length()` is `sqrt(x * x + y * y)`; it is evaluated here
    the same way (the TypeScript `Vector2Dd.length()` uses `Math.hypot`).
    */
    private static vertexLength(p: Vector2Dd): number {
        return JavaMath.sqrt(p.x * p.x + p.y * p.y);
    }

    /** C++ `Vector2Dd::operator==`: exact comparison of the coordinates. */
    private static sameVertex(a: Vector2Dd, b: Vector2Dd): boolean {
        return a.x === b.x && a.y === b.y;
    }

    public getCircumradius(): number {
        let r = 0.0;
        for (let i = 0; i < this.base.size(); i++) {
            const d = Prism.vertexLength(this.base.get(i));
            if (d > r) {
                r = d;
            }
        }
        return r;
    }

    public setCircumradius(value: number): void {
        const r = this.getCircumradius();
        if (!(r > 0.0)) {
            return;
        }
        const factor = value / r;
        const polygon = new ArrayList<Vector2Dd>();
        for (let i = 0; i < this.base.size(); i++) {
            polygon.add(this.base.get(i).multiply(factor));
        }
        this.setBase(polygon);
    }

    private static cross(ax: number, ay: number, bx: number, by: number): number {
        return ax * by - ay * bx;
    }

    /**
    @return true if the closed segments ab and cd share a point
    */
    private static segmentsIntersect(a: Vector2Dd, b: Vector2Dd, c: Vector2Dd, d: Vector2Dd): boolean {
        const d1 = Prism.cross(b.x - a.x, b.y - a.y, c.x - a.x, c.y - a.y);
        const d2 = Prism.cross(b.x - a.x, b.y - a.y, d.x - a.x, d.y - a.y);
        const d3 = Prism.cross(d.x - c.x, d.y - c.y, a.x - c.x, a.y - c.y);
        const d4 = Prism.cross(d.x - c.x, d.y - c.y, b.x - c.x, b.y - c.y);

        if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
            return true;
        }
        // Collinear cases: an endpoint on the other segment
        const minX1 = JavaMath.min(a.x, b.x);
        const maxX1 = JavaMath.max(a.x, b.x);
        const minY1 = JavaMath.min(a.y, b.y);
        const maxY1 = JavaMath.max(a.y, b.y);
        const minX2 = JavaMath.min(c.x, d.x);
        const maxX2 = JavaMath.max(c.x, d.x);
        const minY2 = JavaMath.min(c.y, d.y);
        const maxY2 = JavaMath.max(c.y, d.y);
        if (d1 === 0 && c.x >= minX1 && c.x <= maxX1 && c.y >= minY1 && c.y <= maxY1) {
            return true;
        }
        if (d2 === 0 && d.x >= minX1 && d.x <= maxX1 && d.y >= minY1 && d.y <= maxY1) {
            return true;
        }
        if (d3 === 0 && a.x >= minX2 && a.x <= maxX2 && a.y >= minY2 && a.y <= maxY2) {
            return true;
        }
        if (d4 === 0 && b.x >= minX2 && b.x <= maxX2 && b.y >= minY2 && b.y <= maxY2) {
            return true;
        }
        return false;
    }

    private updateBaseCache(): void {
        const n = this.base.size();
        this.validBase = n >= 3;
        this.minX = this.minY = Double.POSITIVE_INFINITY;
        this.maxX = this.maxY = Double.NEGATIVE_INFINITY;
        for (let i = 0; i < n; i++) {
            const p = this.base.get(i);
            if (!Double.isFinite(p.x) || !Double.isFinite(p.y)) {
                this.validBase = false;
            }
            this.minX = JavaMath.min(this.minX, p.x);
            this.minY = JavaMath.min(this.minY, p.y);
            this.maxX = JavaMath.max(this.maxX, p.x);
            this.maxY = JavaMath.max(this.maxY, p.y);
        }
        if (!this.validBase) {
            return;
        }

        // Non-zero area (setBase made it counterclockwise)
        let twiceArea = 0.0;
        for (let i = 0; i < n; i++) {
            const a = this.base.get(i);
            const b = this.base.get((i + 1) % n);
            twiceArea += a.x * b.y - b.x * a.y;
        }
        if (!(twiceArea > 0.0)) {
            this.validBase = false;
            return;
        }

        // Simple polygon: no repeated consecutive vertices, adjacent edges
        // only share their common vertex, other edges share nothing
        for (let i = 0; i < n && this.validBase; i++) {
            const a = this.base.get(i);
            const b = this.base.get((i + 1) % n);
            if (Prism.sameVertex(a, b)) {
                this.validBase = false;
                break;
            }
            for (let j = i + 1; j < n; j++) {
                const c = this.base.get(j);
                const d = this.base.get((j + 1) % n);
                const adjacent = j === i + 1 || (i === 0 && j === n - 1);
                if (adjacent) {
                    // Shared vertex: they may not fold back over each other
                    const shared = j === i + 1 ? b : a;
                    const other1 = j === i + 1 ? a : b;
                    const other2 = j === i + 1 ? d : c;
                    const side = Prism.cross(
                        other1.x - shared.x,
                        other1.y - shared.y,
                        other2.x - shared.x,
                        other2.y - shared.y,
                    );
                    const dot =
                        (other1.x - shared.x) * (other2.x - shared.x) + (other1.y - shared.y) * (other2.y - shared.y);
                    if (side === 0 && dot > 0) {
                        this.validBase = false;
                        break;
                    }
                } else if (Prism.segmentsIntersect(a, b, c, d)) {
                    this.validBase = false;
                    break;
                }
            }
        }
    }

    /**
    @return true if the base is a simple polygon of at least three vertices
    and non-zero area, and the height is positive (and everything finite)
    */
    public hasValidParameters(): boolean {
        return this.validBase && Double.isFinite(this.height) && this.height > 0.0;
    }

    /**
    Triangulates the base by ear clipping.
    @param outTriangles receives three indices of base vertices per
    triangle, counterclockwise seen from +Z
    @return the number of triangles (number of sides - 2), or 0 if the base
    is not valid
    */
    public triangulateBase(outTriangles: ArrayList<number>): number {
        outTriangles.clear();
        if (!this.validBase) {
            return 0;
        }
        const remaining = new ArrayList<number>();
        for (let i = 0; i < this.base.size(); i++) {
            remaining.add(i);
        }
        let triangles = 0;
        while (remaining.size() > 3) {
            const m = remaining.size();
            let clipped = false;
            for (let i = 0; i < m && !clipped; i++) {
                const a = this.base.get(remaining.get((i + m - 1) % m));
                const b = this.base.get(remaining.get(i));
                const c = this.base.get(remaining.get((i + 1) % m));
                // Convex corner of a counterclockwise polygon
                if (!(Prism.cross(b.x - a.x, b.y - a.y, c.x - b.x, c.y - b.y) > 0)) {
                    continue;
                }
                let ear = true;
                for (let j = 0; j < m && ear; j++) {
                    if (j === i || j === (i + m - 1) % m || j === (i + 1) % m) {
                        continue;
                    }
                    const p = this.base.get(remaining.get(j));
                    if (Prism.sameVertex(p, a) || Prism.sameVertex(p, b) || Prism.sameVertex(p, c)) {
                        continue;
                    }
                    const s1 = Prism.cross(b.x - a.x, b.y - a.y, p.x - a.x, p.y - a.y);
                    const s2 = Prism.cross(c.x - b.x, c.y - b.y, p.x - b.x, p.y - b.y);
                    const s3 = Prism.cross(a.x - c.x, a.y - c.y, p.x - c.x, p.y - c.y);
                    if (s1 >= 0 && s2 >= 0 && s3 >= 0) {
                        ear = false;
                    }
                }
                if (ear) {
                    outTriangles.add(remaining.get((i + m - 1) % m));
                    outTriangles.add(remaining.get(i));
                    outTriangles.add(remaining.get((i + 1) % m));
                    remaining.remove(i);
                    triangles++;
                    clipped = true;
                }
            }
            if (!clipped) {
                // Only possible with rounding on nearly degenerate bases
                outTriangles.clear();
                return 0;
            }
        }
        outTriangles.add(remaining.get(0));
        outTriangles.add(remaining.get(1));
        outTriangles.add(remaining.get(2));
        return triangles + 1;
    }

    //= Interior of the prism along a line ===============================

    /**
    @param closestEdge one-element output holder receiving the index of the
    closest edge
    @return the distance from (x, y) to the closest edge of the base
    */
    private distanceToBaseBoundary(x: number, y: number, closestEdge: number[]): number {
        let best = Double.POSITIVE_INFINITY;
        const n = this.base.size();
        closestEdge[0] = 0;
        for (let i = 0; i < n; i++) {
            const a = this.base.get(i);
            const b = this.base.get((i + 1) % n);
            const ex = b.x - a.x;
            const ey = b.y - a.y;
            const lengthSquared = ex * ex + ey * ey;
            let s = lengthSquared > 0 ? ((x - a.x) * ex + (y - a.y) * ey) / lengthSquared : 0.0;
            if (s < 0) {
                s = 0;
            } else if (s > 1) {
                s = 1;
            }
            const dx = x - (a.x + s * ex);
            const dy = y - (a.y + s * ey);
            const d = JavaMath.sqrt(dx * dx + dy * dy);
            if (d < best) {
                best = d;
                closestEdge[0] = i;
            }
        }
        return best;
    }

    /**
    @return 1 if (x, y) is strictly inside the base, 0 if it is on its
    boundary and -1 if it is outside
    */
    private classifyBasePoint(x: number, y: number): number {
        const n = this.base.size();
        let inside = false;
        for (let i = 0; i < n; i++) {
            const a = this.base.get(i);
            const b = this.base.get((i + 1) % n);
            const side = Prism.cross(b.x - a.x, b.y - a.y, x - a.x, y - a.y);
            if (
                side === 0 &&
                x >= JavaMath.min(a.x, b.x) &&
                x <= JavaMath.max(a.x, b.x) &&
                y >= JavaMath.min(a.y, b.y) &&
                y <= JavaMath.max(a.y, b.y)
            ) {
                return 0;
            }
            // Crossing number with the half-open rule on y
            if (a.y > y !== b.y > y) {
                const xCross = a.x + ((y - a.y) * (b.x - a.x)) / (b.y - a.y);
                if (xCross > x) {
                    inside = !inside;
                }
            }
        }
        return inside ? 1 : -1;
    }

    /**
    Parts of the line `origin + t * direction` (for every t) inside the
    prism, in increasing order. Empty intervals are discarded, so a line
    that only touches the prism has none.
    @param out receives the intervals
    @return false if the line runs along a face of the prism over an
    interval (it lies on the plane of a cap and crosses the base, or on the
    plane of a wall and crosses the slab), so the interior along it is not
    well defined; `out` then has the intervals as computed with the line
    displaced to one side, which the classic interface still uses
    */
    private computeInteriorIntervals(origin: Vector3Dd, direction: Vector3Dd, out: ArrayList<PrismInterval>): boolean {
        out.clear();
        let welldefined = true;

        //- Slab 0 <= z <= height ------------------------------------------
        let slab: PrismInterval;
        let slabOnCap = false;
        const oz = origin.z();
        const dz = direction.z();
        if (dz === 0.0) {
            if (oz === 0.0 || oz === this.height) {
                slabOnCap = true;
            } else if (!(oz > 0.0 && oz < this.height)) {
                return true;
            }
            slab = new PrismInterval(
                Double.NEGATIVE_INFINITY,
                Prism.SURFACE_NONE,
                Double.POSITIVE_INFINITY,
                Prism.SURFACE_NONE,
            );
        } else {
            const tBottom = -oz / dz;
            const tTop = (this.height - oz) / dz;
            if (tBottom < tTop) {
                slab = new PrismInterval(tBottom, Prism.SURFACE_BOTTOM, tTop, Prism.SURFACE_TOP);
            } else {
                slab = new PrismInterval(tTop, Prism.SURFACE_TOP, tBottom, Prism.SURFACE_BOTTOM);
            }
        }

        //- Parts of the projected line inside the base --------------------
        const planar = new ArrayList<PrismInterval>();
        const ox = origin.x();
        const oy = origin.y();
        const dx = direction.x();
        const dy = direction.y();
        const n = this.base.size();

        if (dx === 0.0 && dy === 0.0) {
            // Parallel to the walls: everything or nothing
            const c = this.classifyBasePoint(ox, oy);
            if (c === 0) {
                return false;
            }
            if (c > 0) {
                planar.add(
                    new PrismInterval(
                        Double.NEGATIVE_INFINITY,
                        Prism.SURFACE_NONE,
                        Double.POSITIVE_INFINITY,
                        Prism.SURFACE_NONE,
                    ),
                );
            }
        } else {
            // Crossings with the edges. A vertex on the line is taken as if it
            // were displaced to the positive side of the line (simulation of
            // simplicity), so the crossings always pair as entering / leaving.
            const crossings = new ArrayList<PrismEdgeCrossing>();
            for (let i = 0; i < n; i++) {
                const a = this.base.get(i);
                const b = this.base.get((i + 1) % n);
                const sideA = Prism.cross(dx, dy, a.x - ox, a.y - oy);
                const sideB = Prism.cross(dx, dy, b.x - ox, b.y - oy);
                if (sideA === 0.0 && sideB === 0.0) {
                    // The edge is on the line: the line (which crosses the
                    // slab) runs along its wall
                    welldefined = false;
                    continue;
                }
                if (sideA < 0.0 === sideB < 0.0) {
                    continue;
                }
                const ex = b.x - a.x;
                const ey = b.y - a.y;
                const t = Prism.cross(a.x - ox, a.y - oy, ex, ey) / Prism.cross(dx, dy, ex, ey);
                // Insertion in increasing order of t
                let position = crossings.size();
                crossings.add(new PrismEdgeCrossing(t, i));
                while (position > 0 && crossings.get(position - 1).lineParameter > t) {
                    crossings.set(position, crossings.get(position - 1));
                    position--;
                }
                crossings.set(position, new PrismEdgeCrossing(t, i));
            }
            for (let i = 0; i + 1 < crossings.size(); i += 2) {
                const start = crossings.get(i).lineParameter;
                const end = crossings.get(i + 1).lineParameter;
                if (!(start < end)) {
                    // The line only touches a vertex
                    continue;
                }
                const startSurface = Prism.SURFACE_FIRST_WALL + crossings.get(i).edge;
                const endSurface = Prism.SURFACE_FIRST_WALL + crossings.get(i + 1).edge;
                if (planar.size() > 0 && planar.get(planar.size() - 1).end === start) {
                    // The line goes through a reflex vertex: no boundary there
                    planar.get(planar.size() - 1).end = end;
                    planar.get(planar.size() - 1).endSurface = endSurface;
                } else {
                    planar.add(new PrismInterval(start, startSurface, end, endSurface));
                }
            }
        }

        //- Slab intersected with the planar intervals ---------------------
        for (let i = 0; i < planar.size(); i++) {
            const body = planar.get(i).copy();
            if (slab.start > body.start) {
                body.start = slab.start;
                body.startSurface = slab.startSurface;
            }
            if (slab.end < body.end) {
                body.end = slab.end;
                body.endSurface = slab.endSurface;
            }
            if (body.start < body.end) {
                if (slabOnCap) {
                    // The line lies on the plane of a cap, over the base
                    welldefined = false;
                }
                out.add(body);
            }
        }
        if (slabOnCap) {
            // A line on a cap only touches the prism
            out.clear();
        }
        return welldefined;
    }

    /**
    First point of the surface of the prism hit by a ray, after `minT`.
    @param surface one-element output holder receiving the surface hit
    @param overlap one-element output holder telling if the ray runs along a
    face (see `computeInteriorIntervals`)
    @return the parameter of the hit, or NaN if there is none
    */
    private firstSurfaceHit(ray: Ray, minT: number, surface: number[], overlap: boolean[]): number {
        const intervals = new ArrayList<PrismInterval>();
        overlap[0] = !this.computeInteriorIntervals(ray.getOrigin(), ray.getDirection(), intervals);
        for (let i = 0; i < intervals.size(); i++) {
            const interval = intervals.get(i);
            if (interval.start > minT) {
                surface[0] = interval.startSurface;
                return interval.start;
            }
            if (interval.end > minT) {
                surface[0] = interval.endSurface;
                return interval.end;
            }
        }
        return Double.NaN;
    }

    //= Classic ray intersection interface ===============================

    /**
    C++ overloads `Ray* doIntersectionFirstHit(const Ray& inOutRay)`
    (returns the ray with the distance to the hit, or null if the ray does
    not hit current Prism) and
    `bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)`.
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit();
            if (this.doIntersectionFirstHit(inOutRay, hit) && hit.getRay() !== null) {
                return hit.getRay();
            }
            return null;
        }

        if (!this.hasValidParameters()) {
            return false;
        }
        const surface = [Prism.SURFACE_NONE];
        const overlap = [false];
        const t = this.firstSurfaceHit(inOutRay, VSDK.EPSILON, surface, overlap);

        if (Double.isNaN(t)) {
            return false;
        }
        if (outHit !== null) {
            if (outHit.shouldStoreRay() || outHit.needsAnySurfaceData()) {
                outHit.setRay(inOutRay.withT(t));
            } else {
                outHit.setHitDistance(t);
            }
            if (outHit.needsAnySurfaceData()) {
                this.fillSurfaceData(inOutRay, t, surface[0]!, outHit);
            }
        }
        return true;
    }

    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if (outData === null) {
            return;
        }
        const p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
        this.fillSurfaceData(inRay, inT, this.surfaceAt(p), outData);
    }

    /**
    @return the surface of the prism nearest to a point of its boundary
    */
    private surfaceAt(p: Vector3Dd): number {
        const edge = [0];
        const dWall = this.distanceToBaseBoundary(p.x(), p.y(), edge);
        const dBottom = JavaMath.abs(p.z());
        const dTop = JavaMath.abs(p.z() - this.height);
        let surface = Prism.SURFACE_BOTTOM;
        let best = dBottom;

        if (dTop < best) {
            best = dTop;
            surface = Prism.SURFACE_TOP;
        }
        if (dWall < best) {
            surface = Prism.SURFACE_FIRST_WALL + edge[0]!;
        }
        return surface;
    }

    private fillSurfaceData(ray: Ray, t: number, surface: number, outData: RayHit): void {
        outData.beginDetailEvaluation();
        const p = ray.getOrigin().add(ray.getDirection().multiply(t));
        let normal: Vector3Dd;
        let tangent: Vector3Dd;
        let u: number;
        let v: number;
        const width = this.maxX - this.minX;
        const depth = this.maxY - this.minY;

        if (surface === Prism.SURFACE_TOP || surface === Prism.SURFACE_BOTTOM) {
            normal = new Vector3Dd(0, 0, surface === Prism.SURFACE_TOP ? 1 : -1);
            tangent = new Vector3Dd(1, 0, 0);
            u = width > 0 ? (p.x() - this.minX) / width : 0;
            v = depth > 0 ? (p.y() - this.minY) / depth : 0;
        } else {
            const n = this.base.size();
            let edge = surface - Prism.SURFACE_FIRST_WALL;
            if (edge < 0 || edge >= n) {
                edge = 0;
            }
            const a = this.base.get(edge);
            const b = this.base.get((edge + 1) % n);
            const ex = b.x - a.x;
            const ey = b.y - a.y;
            const length = JavaMath.sqrt(ex * ex + ey * ey);
            // Counterclockwise base: the exterior is on the right of each edge
            normal = new Vector3Dd(ey / length, -ex / length, 0);
            tangent = new Vector3Dd(ex / length, ey / length, 0);
            const s = ((p.x() - a.x) * ex + (p.y() - a.y) * ey) / (length * length);
            u = (edge + s) / n;
            v = this.height > 0 ? p.z() / this.height : 0;
        }

        if (outData.needsPoint()) {
            outData.point = p;
            outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if (outData.needsNormal()) {
            outData.normal = normal;
            outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
        }
        if (outData.needsTangent()) {
            outData.tangent = tangent;
            outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
        if (outData.needsTextureCoordinates()) {
            outData.u = u;
            outData.v = v;
            outData.markDetailAvailable(RayHit.DETAIL_UV);
        }
    }

    //= Ray crossing and classification interface ========================

    /**
    C++ port style of `Tube::validateQuery`.
    @return null if the query and the ray can be processed, or the status
    explaining why they can not
    */
    private validateQuery(ray: Ray, query: RayCrossingQuery): RayFirstHitStatus | null {
        if (
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach)
        ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (query.minimumChordSquared !== 0 || query.minimumExteriorApproach !== Double.NEGATIVE_INFINITY) {
            return RayFirstHitStatus.Unsupported;
        }
        const o = ray.getOrigin();
        const d = ray.getDirection();
        if (
            !Double.isFinite(o.x()) ||
            !Double.isFinite(o.y()) ||
            !Double.isFinite(o.z()) ||
            !Double.isFinite(d.x()) ||
            !Double.isFinite(d.y()) ||
            !Double.isFinite(d.z()) ||
            (d.x() === 0 && d.y() === 0 && d.z() === 0)
        ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (!this.hasValidParameters()) {
            return RayFirstHitStatus.InvalidInput;
        }
        return null;
    }

    public override emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        const invalid = this.validateQuery(ray, query);
        if (invalid !== null) {
            if (invalid === RayFirstHitStatus.Unsupported) {
                return RayCrossingSupport.Unsupported;
            }
            return RayCrossingSupport.InvalidInput;
        }

        const intervals = new ArrayList<PrismInterval>();
        if (!this.computeInteriorIntervals(ray.getOrigin(), ray.getDirection(), intervals)) {
            return RayCrossingSupport.Indeterminate;
        }
        for (let i = 0; i < intervals.size(); i++) {
            if (!Double.isFinite(intervals.get(i).start) || !Double.isFinite(intervals.get(i).end)) {
                return RayCrossingSupport.Indeterminate;
            }
        }
        for (let i = 0; i < intervals.size(); i++) {
            const entry = intervals.get(i).start;
            const exit = intervals.get(i).end;
            if (entry >= query.minT && entry <= query.maxT) {
                const crossing = new RayCrossing(entry, this, RayCrossingKind.Entering);
                crossing.surfaceIndex = intervals.get(i).startSurface;
                sink.crossing(crossing);
            }
            if (exit >= query.minT && exit <= query.maxT) {
                const crossing = new RayCrossing(exit, this, RayCrossingKind.Leaving);
                crossing.surfaceIndex = intervals.get(i).endSurface;
                sink.crossing(crossing);
            }
        }
        return RayCrossingSupport.Supported;
    }

    public override queryFirstHit(
        ray: Ray,
        query: RayCrossingQuery,
        result: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (result !== null) {
            result.clear();
        }
        if (boundary !== null) {
            boundary.assign(new RayBoundary());
        }
        const invalid = this.validateQuery(ray, query);
        if (invalid !== null) {
            return invalid;
        }

        // The first boundary at or after minT: the surface hit is searched
        // strictly after a parameter slightly smaller than minT
        const surface = [Prism.SURFACE_NONE];
        const overlap = [false];
        const t = this.firstSurfaceHit(ray, JavaMath.nextAfter(query.minT, Double.NEGATIVE_INFINITY), surface, overlap);

        if (overlap[0]) {
            return RayFirstHitStatus.Indeterminate;
        }
        if (Double.isNaN(t) || t > query.maxT) {
            return RayFirstHitStatus.Miss;
        }
        if (boundary !== null) {
            boundary.assign(new RayBoundary(t, this, false, surface[0]!));
        }
        if (result !== null) {
            result.setHitDistance(t);
            if (result.shouldStoreRay()) {
                result.setRay(ray.withT(t));
            }
            if (result.needsAnySurfaceData()) {
                this.fillSurfaceData(ray, t, surface[0]!, result);
            }
        }
        return RayFirstHitStatus.Hit;
    }

    public override querySurfaceDetail(ray: Ray, boundary: RayBoundary, hit: RayHit | null): RayFirstHitStatus {
        if (hit !== null) {
            hit.clear();
        }
        if (hit === null || boundary.geometry !== this || !Double.isFinite(boundary.t)) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (boundary.getInstanceCount() !== 0) {
            return RayFirstHitStatus.Unsupported;
        }
        const found = new RayBoundary();
        const status = this.queryFirstHit(ray, new RayCrossingQuery(boundary.t, boundary.t), null, found);
        if (status !== RayFirstHitStatus.Hit) {
            return status;
        }
        hit.setHitDistance(boundary.t);
        if (hit.shouldStoreRay()) {
            hit.setRay(ray.withT(boundary.t));
        }
        const surface =
            boundary.surfaceIndex >= Prism.SURFACE_BOTTOM &&
            boundary.surfaceIndex < Prism.SURFACE_FIRST_WALL + this.base.size()
                ? boundary.surfaceIndex
                : found.surfaceIndex;
        this.fillSurfaceData(ray, boundary.t, surface, hit);
        if (
            boundary.normalReversed &&
            hit.needsNormal() &&
            (hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL) !== 0
        ) {
            hit.normal = hit.normal.multiply(-1);
        }
        return RayFirstHitStatus.Hit;
    }

    /**
    Classifies the ray against the interior of the prism, following the
    contract of `Geometry.classifyRay` (the one of [ROTH1982] CSG): the
    first element of `result.insideSegments` tells if the ray starts (at
    `query.minT`) inside the prism, and each boundary toggles it. A ray
    running along a face is `Indeterminate`.
    */
    public override classifyRay(
        ray: Ray,
        query: RayCrossingQuery,
        result: RaySolidClassification,
    ): RaySolidQueryStatus {
        result.clear();
        if (query.minT >= query.maxT) {
            return RaySolidQueryStatus.InvalidInput;
        }
        const invalid = this.validateQuery(ray, query);
        if (invalid !== null) {
            if (invalid === RayFirstHitStatus.Unsupported) {
                return RaySolidQueryStatus.Unsupported;
            }
            return RaySolidQueryStatus.InvalidInput;
        }
        const intervals = new ArrayList<PrismInterval>();
        if (!this.computeInteriorIntervals(ray.getOrigin(), ray.getDirection(), intervals)) {
            return RaySolidQueryStatus.Indeterminate;
        }
        let startsInside = false;

        for (let i = 0; i < intervals.size(); i++) {
            if (query.minT > intervals.get(i).start && query.minT <= intervals.get(i).end) {
                startsInside = true;
            }
        }
        if (!result.insideSegments.add(startsInside)) {
            return RaySolidQueryStatus.Indeterminate;
        }
        for (let i = 0; i < intervals.size(); i++) {
            const entry = intervals.get(i).start;
            const exit = intervals.get(i).end;
            if (entry >= query.minT && entry <= query.maxT) {
                if (
                    !result.boundaries.add(new RaySolidBoundary(entry, this, false, intervals.get(i).startSurface)) ||
                    !result.insideSegments.add(true)
                ) {
                    result.clear();
                    return RaySolidQueryStatus.Indeterminate;
                }
            }
            if (exit >= query.minT && exit <= query.maxT) {
                if (
                    !result.boundaries.add(new RaySolidBoundary(exit, this, false, intervals.get(i).endSurface)) ||
                    !result.insideSegments.add(false)
                ) {
                    result.clear();
                    return RaySolidQueryStatus.Indeterminate;
                }
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    //= Containment ======================================================

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Distances are measured perpendicular to the
    caps and to the walls (in the plane z = 0).
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        if (!this.hasValidParameters()) {
            return Prism.OUTSIDE;
        }
        const z = p.z();
        if (z < -distanceTolerance || z > this.height + distanceTolerance) {
            return Prism.OUTSIDE;
        }
        const edge = [0];
        const dWall = this.distanceToBaseBoundary(p.x(), p.y(), edge);
        const c = this.classifyBasePoint(p.x(), p.y());

        if (c < 0) {
            return dWall <= distanceTolerance ? Prism.LIMIT : Prism.OUTSIDE;
        }
        if (c > 0 && dWall > distanceTolerance && z > distanceTolerance && z < this.height - distanceTolerance) {
            return Prism.INSIDE;
        }
        return Prism.LIMIT;
    }

    public override classifyPoint(p: Vector3Dd, distanceTolerance: number): GeometryContainment {
        if (
            !Double.isFinite(distanceTolerance) ||
            distanceTolerance < 0 ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())
        ) {
            return GeometryContainment.InvalidInput;
        }
        if (!this.hasValidParameters()) {
            return GeometryContainment.InvalidInput;
        }
        const c = this.doContainmentTest(p, distanceTolerance);
        if (c === Prism.INSIDE) {
            return GeometryContainment.Inside;
        }
        if (c === Prism.LIMIT) {
            return GeometryContainment.Boundary;
        }
        return GeometryContainment.Outside;
    }

    //= Bounds ===========================================================

    /** Finite bounds: the box of the base, and 0 <= z <= height. */
    public override getBounds(result: GeometryBounds): void {
        if (!this.hasValidParameters()) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(new Vector3Dd(this.minX, this.minY, 0), new Vector3Dd(this.maxX, this.maxY, this.height));
    }

    public override getMinMax(): Float64Array {
        const minmax = new Float64Array(6);

        minmax[0] = this.minX;
        minmax[1] = this.minY;
        minmax[2] = 0;
        minmax[3] = this.maxX;
        minmax[4] = this.maxY;
        minmax[5] = this.height;
        return minmax;
    }

    //= Boundary representation ==========================================

    /**
    The prism is built as in [MANT1988] 12.3.1 (program 12.4): a planar
    lamina of the base polygon (`mvfs`, `smev` along the vertices, `smef`)
    whose face 1 (looking to +Z) is swept `height` units along +Z.
    @return a new boundary representation of the prism
    */
    public override exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid {
        return this.buildPolyhedralBoundedSolid();
    }

    private buildPolyhedralBoundedSolid(): PolyhedralBoundedSolid {
        const solid = new PolyhedralBoundedSolid();
        const n = this.base.size();
        let i: number;

        PolyhedralBoundedSolidEulerOperators.mvfs(
            solid,
            new Vector3Dd(this.base.get(0).x, this.base.get(0).y, 0.0),
            1,
            1,
        );
        for (i = 1; i < n; i++) {
            PolyhedralBoundedSolidEulerOperators.smev(
                solid,
                1,
                i,
                i + 1,
                new Vector3Dd(this.base.get(i).x, this.base.get(i).y, 0.0),
            );
        }
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, n, 1, 2);
        let t = new Matrix4x4d();
        t = t.translation(0.0, 0.0, this.height);
        PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFace(solid, solid.findFace(1), t);
        return solid;
    }
}
