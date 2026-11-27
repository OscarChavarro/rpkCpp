//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

import { VSDK } from "../../../../common/VSDK.js";
import { Logger } from "../../../../common/logging/Logger.js";
import { Vertex2D } from "../../element/Vertex2D.js";
import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import { CircularArrayList } from "../../../../common/dataStructures/CircularArrayList.js";
import { Ray } from "../../element/Ray.js";
import { Geometry } from "../../Geometry.js";
import { RayHit } from "../../element/RayHit.js";
import type { InfinitePlane } from "../../surface/InfinitePlane.js";
import { Solid } from "../Solid.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidEdge } from "./nodes/_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";
import { ComputationalGeometry } from "../../../../processing/ComputationalGeometry.js";
import { Polygon2D } from "../../surface/polygon/Polygon2D.js";
import { PolyhedralBoundedSolidNumericPolicy, type ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import { PolyhedralBoundedSolidPredicates } from "./PolyhedralBoundedSolidPredicates.js";

/**
This class encapsulates a polyhedral boundary representation for 2-manifold
solids, as presented in [MANT1988].

As noted in [MANT1988].6.2.1., a "polyhedral model" is a boundary model
that has only planar faces. So, the name of this class `PolyhedralBoundedSolid`
implies that its faces should be planar. However, some intermediate steps
in complex algorithms such as the splitter and the set operators, permits
the use of "special" non-planar faces for "gluing".  Check [MANT1988] book
for more details on that.

As noted in [MANT1988].10.2.1, current implementation of the
`PolyhedralBoundedSolid` class uses a five-level hierarchical data
structure, consisting of:
  - PolyhedralBoundedSolid
  - _PolyhedralBoundedSolidFace
  - _PolyhedralBoundedSolidLoop
  - _PolyhedralBoundedSolidHalfEdge (and _PolyhedralBoundedSolidEdge)
  - _PolyhedralBoundedSolidVertex
Current class forms the root element (Facade) that gives access to faces,
edges and vertices of the model through aggregations in
CircularArrayList's.

Note that this is a quite complex data-structure. Its implementation follows
the strategies outlined on book [MANT1988]. For the sake of clarity, it
was decided to keep most of its internal datastructures public, breaking
so the encapsulation concept. Note that if internal data structures are
made private and accessing get/set methods are provided for them, then
the complexity of algorithms using current data-structure should become
unmanageable, both in terms of code verbosity and bad performance (time
complexity) due to extra calls to a lot of simple methods.
*/
export class PolyhedralBoundedSolid extends Solid {
    public static readonly PLUS = 1;
    public static readonly MINUS = 0;
    private static readonly SOLID_WITH_MESSAGE = "Solid with ";

    //= Main boundary representation solid data structure =============
    private polygonsList: CircularArrayList<_PolyhedralBoundedSolidFace>;
    private edgesList: CircularArrayList<_PolyhedralBoundedSolidEdge>;
    private verticesList: CircularArrayList<_PolyhedralBoundedSolidVertex>;
    private maxVertexId: number;
    private maxFaceId: number;
    private modelIsValid: boolean;

    //=================================================================
    public constructor() {
        super();
        this.polygonsList = new CircularArrayList<_PolyhedralBoundedSolidFace>();
        this.edgesList = new CircularArrayList<_PolyhedralBoundedSolidEdge>();
        this.verticesList = new CircularArrayList<_PolyhedralBoundedSolidVertex>();
        this.maxVertexId = -1;
        this.maxFaceId = -1;
        this.modelIsValid = false;
    }

    //= SUPPORT MACROS FOR BASIC DATA-STRUCTURE MANIPULATION ==========

    /**
    Find the face identified with `id`. Returns null if face not found,
    or current founded face otherwise.
    Build based over function `fface` in program [MANT1988].11.9.
    @param id face id to search.
    @return matching face, or null when not found.
    */
    public findFace(id: number): _PolyhedralBoundedSolidFace | null {
        let i: number;
        let face: _PolyhedralBoundedSolidFace;

        for (i = 0; i < this.polygonsList.size(); i++) {
            face = this.polygonsList.get(i)!;
            if (face.id === id) {
                return face;
            }
        }
        return null;
    }

    /**
    Finds the vertex identified by `id`.
    @param id vertex id to search.
    @return matching vertex, or null when not found.
    */
    public findVertex(id: number): _PolyhedralBoundedSolidVertex | null {
        let i: number;
        let v: _PolyhedralBoundedSolidVertex;

        for (i = 0; i < this.verticesList.size(); i++) {
            v = this.verticesList.get(i)!;
            if (v.id === id) {
                return v;
            }
        }
        return null;
    }

    /**
    Returns the list of faces stored in this solid.
    @return internal face list.
    */
    public getPolygonsList(): CircularArrayList<_PolyhedralBoundedSolidFace> {
        return this.polygonsList;
    }

    /**
    Replaces the internal face list.
    @param polygonsList new face list.
    */
    public setPolygonsList(polygonsList: CircularArrayList<_PolyhedralBoundedSolidFace>): void {
        this.polygonsList = polygonsList;
    }

    /**
    Returns the list of edges stored in this solid.
    @return internal edge list.
    */
    public getEdgesList(): CircularArrayList<_PolyhedralBoundedSolidEdge> {
        return this.edgesList;
    }

    /**
    Replaces the internal edge list.
    @param edgesList new edge list.
    */
    public setEdgesList(edgesList: CircularArrayList<_PolyhedralBoundedSolidEdge>): void {
        this.edgesList = edgesList;
    }

    /**
    Returns the list of vertices stored in this solid.
    @return internal vertex list.
    */
    public getVerticesList(): CircularArrayList<_PolyhedralBoundedSolidVertex> {
        return this.verticesList;
    }

    /**
    Replaces the internal vertex list.
    @param verticesList new vertex list.
    */
    public setVerticesList(verticesList: CircularArrayList<_PolyhedralBoundedSolidVertex>): void {
        this.verticesList = verticesList;
    }

    //=================================================================

    /**
    This method gives access to the higher vertex id used in current solid
    model. This method is useful for higher level modeling operations, as
    noted in section [MANT1988].12.2. Current method (and method getMaxFaceId)
    is build after the function `getmaxnames` of program [MANT1988].12.1.
    @return the maximum id used in vertices set
    */
    public getMaxVertexId(): number {
        return this.maxVertexId;
    }

    public setMaxVertexId(maxVertexId: number): void {
        this.maxVertexId = maxVertexId;
    }

    /**
    This method gives access to the higher face id used in current solid
    model. This method is useful for higher level modeling operations, as
    noted in section [MANT1988].12.2. Current method (and method
    getMaxVertexId) is build after the function `getmaxnames` of program
    [MANT1988].12.1.
    @return the maximum id used on faces set
    */
    public getMaxFaceId(): number {
        return this.maxFaceId;
    }

    public setMaxFaceId(maxFaceId: number): void {
        this.maxFaceId = maxFaceId;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.

    Java overload `doIntersectionFirstHit(Ray inOutRay)`: returns the hit ray
    with updated `t` when intersection exists, or null otherwise.
    @param inRay input ray to test.
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit(RayHit.DETAIL_NONE, true);
            if (this.doIntersectionFirstHit(inRay, hit)) {
                return hit.getRay();
            }
            return null;
        }

        let i: number;
        let minT: number; // Shortest distance founded so far
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(this);

        // Initialization values for search algorithm
        minT = Number.MAX_VALUE;
        let bestInfo: RayHit | null = null;
        let p: Vector3Dd;
        let pos: number;

        for (i = 0; i < this.polygonsList.size(); i++) {
            const ray = new Ray(inRay);
            const face = this.polygonsList.get(i)!;
            const containingPlane = face.getContainingPlane();
            if (containingPlane === null) {
                continue;
            }
            const planeHit = new RayHit();
            if (containingPlane.doIntersectionFirstHit(ray, planeHit)) {
                let hit = planeHit.getRay()!;
                if (hit.getT() < minT) {
                    hit = hit.withDirection(hit.getDirection().normalized());
                    p = hit.getOrigin().add(hit.getDirection().multiply(hit.getT()));
                    pos = PolyhedralBoundedSolid.testPointInsideForRayIntersection(
                        face,
                        p,
                        numericContext.bigEpsilon(),
                    );
                    if (pos === Geometry.INSIDE || pos === Geometry.LIMIT) {
                        minT = hit.getT();
                        bestInfo = new RayHit(planeHit.getRequiredDetailMask());
                        bestInfo.clone(planeHit);
                    }
                }
            }
        }

        if (bestInfo === null) {
            return false;
        }
        if (outHit !== null) {
            outHit.clone(bestInfo);
            outHit.setRay(inRay.withT(minT));
        }
        return true;
    }

    private static dropCoordinate(input: Vector3Dd, coordinate: number): Vector3Dd {
        switch (coordinate) {
            case 1:
                return new Vector3Dd(input.y(), input.z(), 0);
            case 2:
                return new Vector3Dd(input.x(), input.z(), 0);
            default:
                return new Vector3Dd(input.x(), input.y(), 0);
        }
    }

    private static dominantCoordinateForFace(face: _PolyhedralBoundedSolidFace): number {
        const n = face.getContainingPlane()!.getNormal();

        if (Math.abs(n.x()) >= Math.abs(n.y()) && Math.abs(n.x()) >= Math.abs(n.z())) {
            return 1;
        }
        if (Math.abs(n.y()) >= Math.abs(n.x()) && Math.abs(n.y()) >= Math.abs(n.z())) {
            return 2;
        }
        return 3;
    }

    private static testPointInsideForRayIntersection(
        face: _PolyhedralBoundedSolidFace | null,
        point: Vector3Dd,
        tolerance: number,
    ): number {
        let dominantCoordinate: number;
        let insideLoopCount: number;
        let i: number;
        let projectedPoint: Vector3Dd;

        if (face === null || face.getContainingPlane() === null) {
            return Geometry.OUTSIDE;
        }

        dominantCoordinate = PolyhedralBoundedSolid.dominantCoordinateForFace(face);
        projectedPoint = PolyhedralBoundedSolid.dropCoordinate(point, dominantCoordinate);
        const projectedPoint2D = new Vertex2D(projectedPoint.x(), projectedPoint.y());
        insideLoopCount = 0;

        for (i = 0; i < face.boundariesList.size(); i++) {
            const loop = face.boundariesList.get(i)!;
            let he = loop.boundaryStartHalfEdge;
            let loopStatus: number;

            if (he === null) {
                return Geometry.OUTSIDE;
            }
            const start = he;
            const projectedLoopVertices: Vertex2D[] = [];

            do {
                if (Vector3Dd.distance(point, he.startingVertex.position) < 2 * tolerance) {
                    return Geometry.LIMIT;
                }
                if (
                    ComputationalGeometry.lineSegmentContainmentTest(
                        he.startingVertex.position,
                        he.next()!.startingVertex.position,
                        point,
                        tolerance,
                    ) === Geometry.LIMIT
                ) {
                    return Geometry.LIMIT;
                }

                projectedPoint = PolyhedralBoundedSolid.dropCoordinate(he.startingVertex.position, dominantCoordinate);
                projectedLoopVertices.push(new Vertex2D(projectedPoint.x(), projectedPoint.y()));
                he = he.next()!;
            } while (he !== start);

            loopStatus = Polygon2D.isPointInsideContour(projectedPoint2D, projectedLoopVertices);
            if (loopStatus === 0) {
                return Geometry.LIMIT;
            }
            if (loopStatus > 0) {
                insideLoopCount++;
            }
        }

        return insideLoopCount % 2 === 1 ? Geometry.INSIDE : Geometry.OUTSIDE;
    }

    /**
    Fills `outData` with intersection details for the provided ray and distance.
    @param inRay input ray.
    @param inT ray parameter used as candidate intersection distance.
    @param outData output hit information container.
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if (outData === null) {
            return;
        }
        this.doIntersectionFirstHit(inRay.withT(inT), outData);
    }

    /** Needed for supplying the Geometry.getMinMax operation */
    private calculateMinMaxPositions(): Float64Array {
        const minMax = new Float64Array(6);

        let minX = Number.MAX_VALUE;
        let minY = Number.MAX_VALUE;
        let minZ = Number.MAX_VALUE;
        let maxX = -Number.MAX_VALUE;
        let maxY = -Number.MAX_VALUE;
        let maxZ = -Number.MAX_VALUE;
        let i: number;

        for (i = 0; i < this.verticesList.size(); i++) {
            const v = this.verticesList.get(i)!;
            const x = v.position.x();
            const y = v.position.y();
            const z = v.position.z();

            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (z < minZ) minZ = z;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
            if (z > maxZ) maxZ = z;
        }
        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = minZ;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = maxZ;
        return minMax;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.getMinMax.
    @return six-value array with min/max coordinates: `[minX, minY, minZ, maxX, maxY, maxZ]`.
    */
    public override getMinMax(): Float64Array {
        return this.calculateMinMaxPositions();
    }

    /**
    Returns true if the model was validated using
    `PolyhedralBoundedSolidValidationEngine.validateIntermediate` or
    `PolyhedralBoundedSolidValidationEngine.validateStrict`, and validation
    succeeded after the latest geometrical or topological operation.
    @return true when the cached validation flag is valid; false otherwise.
    */
    public isValid(): boolean {
        return this.modelIsValid;
    }

    /** Java package-private. */
    public setValidationState(flag: boolean): void {
        this.modelIsValid = flag;
    }

    /**
    Given `this` and `other` solids, this method erases the `other` solid
    while appending its parts to current one as a new shell. This method
    follows section [MANT1988].12.4.1 and program [MANT1988].12.8.
    @param other solid whose topology is moved into this solid.
    */
    public merge(other: PolyhedralBoundedSolid): void {
        //-----------------------------------------------------------------
        const offsetFacesId = this.getMaxFaceId();
        const offsetVertexId = this.getMaxVertexId();
        let f: _PolyhedralBoundedSolidFace;
        let v: _PolyhedralBoundedSolidVertex;

        //-----------------------------------------------------------------
        while (other.getPolygonsList().size() > 0) {
            f = other.getPolygonsList().get(0)!;
            f.id += offsetFacesId;
            if (f.id > this.maxFaceId) this.maxFaceId = f.id;
            this.polygonsList.add(f);
            other.getPolygonsList().remove(0);
        }
        while (other.getEdgesList().size() > 0) {
            this.edgesList.add(other.getEdgesList().get(0)!);
            other.getEdgesList().remove(0);
        }
        while (other.getVerticesList().size() > 0) {
            v = other.getVerticesList().get(0)!;
            v.id += offsetVertexId;
            if (v.id > this.maxVertexId) this.maxVertexId = v.id;
            this.verticesList.add(v);
            other.getVerticesList().remove(0);
        }
    }

    //= TEXTUAL QUERY OPERATIONS ======================================

    private intPreSpaces(val: number, fieldSize: number): string {
        let remain: number;

        const cad = VSDK.formatNumberWithinZeroes(val, 1);
        remain = fieldSize - cad.length;
        let sb = "";

        for (; remain > 0; remain--) {
            sb += " ";
        }
        sb += cad;
        return sb;
    }

    /// Transient per-frame caches for repeated visibility (quantitative
    /// invisibility) queries. The Appel hidden-line renderer issues tens of
    /// thousands of QI samples against a STATIC solid in a single frame; without
    /// these, every sample recomputed each face's containing plane (a Newell fit
    /// + a face-scale estimate) for every one of the four jittered rays — tens of
    /// millions of redundant plane fits. `beginVisibilityQueries` snapshots the
    /// face planes and tolerance once; `endVisibilityQueries` drops the snapshot.
    /// While active the solid MUST NOT be modified.
    private queryPlaneCache: (InfinitePlane | null)[] | null = null;
    /// Per-face axis-aligned bounding box (6 doubles: minX,minY,minZ,maxX,maxY,
    /// maxZ) used to cull faces a query ray cannot hit before the plane test.
    private queryFaceAabb: Float64Array | null = null;
    private queryNumericContext: ToleranceContext | null = null;

    /**
    Snapshots per-face containing planes and the tolerance context for a batch of
    visibility queries on this (unchanged) solid. Call once before issuing many
    computeQuantitativeInvisibility queries, then endVisibilityQueries when done.
    */
    public beginVisibilityQueries(): void {
        this.queryNumericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(this);
        const faceCount = this.polygonsList.size();
        this.queryPlaneCache = new Array<InfinitePlane | null>(faceCount).fill(null);
        this.queryFaceAabb = new Float64Array(faceCount * 6);
        for (let i = 0; i < faceCount; i++) {
            const face = this.polygonsList.get(i)!;
            this.queryPlaneCache[i] = face.getContainingPlane();
            this.computeFaceAabb(face, i);
        }
    }

    private computeFaceAabb(face: _PolyhedralBoundedSolidFace, index: number): void {
        let minX = Number.MAX_VALUE;
        let minY = Number.MAX_VALUE;
        let minZ = Number.MAX_VALUE;
        let maxX = -Number.MAX_VALUE;
        let maxY = -Number.MAX_VALUE;
        let maxZ = -Number.MAX_VALUE;
        for (let b = 0; b < face.boundariesList.size(); b++) {
            const loop = face.boundariesList.get(b);
            if (loop === null || loop.boundaryStartHalfEdge === null) {
                continue;
            }
            let he: _PolyhedralBoundedSolidHalfEdge | null = loop.boundaryStartHalfEdge;
            const start = he;
            do {
                if (he!.startingVertex !== null) {
                    const q = he!.startingVertex.position;
                    if (q.x() < minX) minX = q.x();
                    if (q.y() < minY) minY = q.y();
                    if (q.z() < minZ) minZ = q.z();
                    if (q.x() > maxX) maxX = q.x();
                    if (q.y() > maxY) maxY = q.y();
                    if (q.z() > maxZ) maxZ = q.z();
                }
                he = he!.next();
            } while (he !== null && he !== start);
        }
        const o = index * 6;
        const aabb = this.queryFaceAabb!;
        aabb[o] = minX;
        aabb[o + 1] = minY;
        aabb[o + 2] = minZ;
        aabb[o + 3] = maxX;
        aabb[o + 4] = maxY;
        aabb[o + 5] = maxZ;
    }

    /**
    Slab test: does the segment from `origin` along unit `direction` for length
    `maxT` reach the cached AABB of face `faceIndex`? A cheap, allocation-free
    reject for faces a query ray cannot hit. Conservative (a small epsilon pad),
    so it never culls a face the ray actually crosses.
    */
    private rayReachesFaceAabb(
        origin: Vector3Dd,
        dirX: number,
        dirY: number,
        dirZ: number,
        maxT: number,
        faceIndex: number,
        pad: number,
    ): boolean {
        const aabb = this.queryFaceAabb!;
        const o = faceIndex * 6;
        let tMin = 0.0;
        let tMax = maxT;
        // X slab
        if (Math.abs(dirX) < 1.0e-12) {
            if (origin.x() < aabb[o]! - pad || origin.x() > aabb[o + 3]! + pad) {
                return false;
            }
        } else {
            let t1 = (aabb[o]! - pad - origin.x()) / dirX;
            let t2 = (aabb[o + 3]! + pad - origin.x()) / dirX;
            if (t1 > t2) {
                const tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;
            if (tMin > tMax) return false;
        }
        // Y slab
        if (Math.abs(dirY) < 1.0e-12) {
            if (origin.y() < aabb[o + 1]! - pad || origin.y() > aabb[o + 4]! + pad) {
                return false;
            }
        } else {
            let t1 = (aabb[o + 1]! - pad - origin.y()) / dirY;
            let t2 = (aabb[o + 4]! + pad - origin.y()) / dirY;
            if (t1 > t2) {
                const tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;
            if (tMin > tMax) return false;
        }
        // Z slab
        if (Math.abs(dirZ) < 1.0e-12) {
            if (origin.z() < aabb[o + 2]! - pad || origin.z() > aabb[o + 5]! + pad) {
                return false;
            }
        } else {
            let t1 = (aabb[o + 2]! - pad - origin.z()) / dirZ;
            let t2 = (aabb[o + 5]! + pad - origin.z()) / dirZ;
            if (t1 > t2) {
                const tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;
            if (tMin > tMax) return false;
        }
        return true;
    }

    /** Releases the visibility-query snapshot taken by beginVisibilityQueries. */
    public endVisibilityQueries(): void {
        this.queryPlaneCache = null;
        this.queryFaceAabb = null;
        this.queryNumericContext = null;
    }

    // Package-private query accessors so PolyhedralBoundedSolidPredicates can
    // reuse the per-frame snapshot (plane cache + face AABBs + tolerance) taken
    // by beginVisibilityQueries, falling back to on-the-fly computation when no
    // snapshot is active.

    /** Java package-private. */
    public cachedFacePlane(faceIndex: number): InfinitePlane | null {
        if (this.queryPlaneCache !== null && faceIndex < this.queryPlaneCache.length) {
            return this.queryPlaneCache[faceIndex]!;
        }
        return this.polygonsList.get(faceIndex)!.getContainingPlane();
    }

    /** Tolerance context for visibility queries: the cached one if a snapshot is
        active, otherwise computed for this solid. Java package-private. */
    public queryToleranceContext(): ToleranceContext {
        return this.queryNumericContext !== null
            ? this.queryNumericContext
            : PolyhedralBoundedSolidNumericPolicy.forSolid(this);
    }

    /** AABB reject for a query ray against face `faceIndex`. Returns true (do not
        cull) when no AABB snapshot is active. Java package-private. */
    public queryRayReachesFace(
        origin: Vector3Dd,
        dirX: number,
        dirY: number,
        dirZ: number,
        maxT: number,
        faceIndex: number,
        pad: number,
    ): boolean {
        if (this.queryFaceAabb === null) {
            return true;
        }
        return this.rayReachesFaceAabb(origin, dirX, dirY, dirZ, maxT, faceIndex, pad);
    }

    /** AABB reject for a query POINT against face `faceIndex` (is the point in
        the face's padded box?). Returns true (do not cull) when no snapshot is
        active. Used to skip the on-surface test for far faces. Java
        package-private. */
    public queryPointNearFace(point: Vector3Dd, faceIndex: number, pad: number): boolean {
        if (this.queryFaceAabb === null) {
            return true;
        }
        const aabb = this.queryFaceAabb;
        const o = faceIndex * 6;
        return (
            point.x() >= aabb[o]! - pad &&
            point.x() <= aabb[o + 3]! + pad &&
            point.y() >= aabb[o + 1]! - pad &&
            point.y() <= aabb[o + 4]! + pad &&
            point.z() >= aabb[o + 2]! - pad &&
            point.z() <= aabb[o + 5]! + pad
        );
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Delegated to the robust predicate layer
    (`PolyhedralBoundedSolidPredicates.classifyPoint`): a point on a face is
    `LIMIT`, any other is classified by a ray-cast parity test.
    (Ported from Java commit 402415f4, as a dependency of the free line
    piercing test of `HiddenLineRenderer`.)
    @param p point to classify, in the space of the solid
    @param distanceTolerance distance to the faces under which the point is
    classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd | null, distanceTolerance: number): number {
        if (p === null) {
            Logger.reportMessage(
                this,
                VSDK.WARNING,
                "doContainmentTest",
                "Null point given: it is classified as OUTSIDE.",
            );
            return PolyhedralBoundedSolid.OUTSIDE;
        }
        return PolyhedralBoundedSolidPredicates.classifyPoint(this, p, distanceTolerance);
    }

    /**
    Check the general interface contract in superclass method
    Geometry.computeQuantitativeInvisibility.

    This is not well understood for cases of intersection with face limits
    (vertices and edges). In some cases, computation of quantitative
    invisibility seems to be failing.
    @param origin ray origin.
    @param p target point.
    @return  the number of front facing surface elements (with
    respect to `origin`) between the `origin` point and the `p` point
    */
    public override computeQuantitativeInvisibility(origin: Vector3Dd, p: Vector3Dd): number {
        // Delegated to the robust predicate layer: QI = the number of times the
        // (straight, pulled-back) line of sight enters this solid's interior
        // before reaching the surface point `p`, computed by a boundary-crossing
        // count whose interval classification uses the robust point-in-solid
        // test (PolyhedralBoundedSolidPredicates). This replaces the earlier
        // perpendicular-jitter majority vote, which fabricated phantom occluders
        // at axis-aligned silhouette corners and missed occluders entered through
        // an edge. The per-frame plane/AABB snapshot taken by
        // beginVisibilityQueries is reused by the predicates for speed.
        return PolyhedralBoundedSolidPredicates.quantitativeInvisibility(this, origin, p);
    }

    /**
    Utility routine used to compare floating values inside the boundary
    representation winged edge data structure, following procedure `comp`
    from program [MANT1988].13.2.
    @param a first value.
    @param b second value.
    @param tolerance comparison tolerance.
    @return 0 if two numbers are nearly equal, 1 if a > b, -1 if a < b
    */
    public static compareValue(a: number, b: number, tolerance: number): number {
        let delta: number;

        delta = Math.abs(a - b);
        if (delta < tolerance) {
            return 0;
        } else if (a > b) {
            return 1;
        }
        return -1;
    }

    /**
    This method get current solid in an "inverted" (geometrical sense) solid.
    Works on half edge data structure by inverting the order of each loop.
    This is an answer to problem [MANT1988].15.6.

    Current implementation does NOT correct face normals. Explicit model
    validation is encouraged after the application of this method.
    */
    public revert(): void {
        let i: number;

        for (i = 0; i < this.polygonsList.size(); i++) {
            this.polygonsList.get(i)!.revert();
        }
    }

    public override toString(): string {
        let msg = "";
        let i: number;
        let j: number;

        msg += "= POLYHEDRAL BOUNDED SOLID STRUCTURE ==========================================\n";
        msg += PolyhedralBoundedSolid.SOLID_WITH_MESSAGE + this.verticesList.size() + " vertices:\n";
        for (i = 0; i < this.verticesList.size(); i++) {
            const v = this.verticesList.get(i)!;
            msg += "  - " + v + "\n";
        }

        msg += PolyhedralBoundedSolid.SOLID_WITH_MESSAGE + this.edgesList.size() + " edges:\n";
        for (i = 0; i < this.edgesList.size(); i++) {
            const e = this.edgesList.get(i)!;
            msg += "  - " + e + "\n";
        }
        msg += PolyhedralBoundedSolid.SOLID_WITH_MESSAGE + this.polygonsList.size() + " faces:\n";

        for (i = 0; i < this.polygonsList.size(); i++) {
            const face = this.polygonsList.get(i)!;
            msg += "  - " + face + "\n";
            for (j = 0; j < face.boundariesList.size(); j++) {
                let he: _PolyhedralBoundedSolidHalfEdge | null;

                msg += "    . Loop " + j + ", with half-edges: \n";
                const loop: _PolyhedralBoundedSolidLoop = face.boundariesList.get(j)!;

                msg +=
                    "      | HeID  | StartVertex | End Vertex | nccw He | pccw He | parentEdge | mirror He | neighbor face\n";
                msg +=
                    "      +-------+-------------+------------+---------+---------+------------+-----------+-------------+\n";

                he = loop.boundaryStartHalfEdge;
                if (he === null) {
                    msg += "<Loop without starting half-edge!>\n";
                    continue;
                }
                const heStart = he;
                do {
                    he = he!.next();
                    if (he === null) {
                        // Loop is not closed!
                        msg += "      |  - (not closed loop)\n";
                        break;
                    }

                    msg +=
                        "      | " +
                        this.intPreSpaces(he.id, 4) +
                        (he === loop.boundaryStartHalfEdge ? "*" : " ") +
                        " | " +
                        this.intPreSpaces(he.startingVertex.id, 11) +
                        " | " +
                        this.intPreSpaces(he.next()!.startingVertex.id, 10) +
                        " | " +
                        this.intPreSpaces(he.next()!.id, 7) +
                        " | " +
                        this.intPreSpaces(he.previous()!.id, 7) +
                        " | ";
                    msg += he.parentEdge !== null ? this.intPreSpaces(he.parentEdge.id, 10) : "    <null>";
                    msg += " | ";
                    if (he.mirrorHalfEdge() !== null) {
                        msg +=
                            this.intPreSpaces(he.mirrorHalfEdge()!.id, 9) +
                            " | " +
                            this.intPreSpaces(he.mirrorHalfEdge()!.parentLoop.parentFace.id, 11) +
                            " | ";
                    } else {
                        msg += " No Mirror Half Edge!   | ";
                    }

                    msg += "\n";
                } while (he !== heStart);
            }
        }
        msg += "= END OF POLYHEDRAL BOUNDED SOLID STRUCTURE ===================================\n";
        return msg;
    }

    public override exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid {
        return this;
    }
}
