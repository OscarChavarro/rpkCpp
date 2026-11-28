import { CLONEABLE, type Cloneable } from "../../../../../java/lang/Cloneable.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Image } from "../../../media/Image.js";
import type { SimpleMaterial } from "../../material/SimpleMaterial.js";
import { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { Triangle } from "../element/Triangle.js";
import type { Vertex } from "../element/Vertex.js";
import { Intersection } from "../element/Intersection.js";
import { Surface } from "./Surface.js";
import { TriangleMeshIntersectionIndex } from "./TriangleMeshIntersectionIndex.js";
import { RayBoundary } from "../element/RayBoundary.js";
import { RayCrossing } from "../element/RayCrossing.js";
import { RayCrossingKind } from "../element/RayCrossingKind.js";
import { RayCrossingQuery } from "../element/RayCrossingQuery.js";
import type { RayCrossingSink } from "../element/RayCrossingSink.js";
import { RayCrossingSupport } from "../element/RayCrossingSupport.js";
import { RayFirstHitStatus } from "../element/RayFirstHitStatus.js";
import type { GeometryBounds } from "../element/GeometryBounds.js";
import type { Geometry } from "../Geometry.js";
import type { InfinitePlane } from "./InfinitePlane.js";
import { VSDK } from "../../../common/VSDK.js";
import { Logger } from "../../../common/logging/Logger.js";
import { ArrayListOfDoubles } from "../../../common/dataStructures/ArrayListOfDoubles.js";
import { ArrayListOfInts } from "../../../common/dataStructures/ArrayListOfInts.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { JavaArray } from "../../../../../java/lang/JavaArray.js";
import { IndexOutOfBoundsException } from "../../../../../java/lang/IndexOutOfBoundsException.js";
import { NullPointerException } from "../../../../../java/lang/NullPointerException.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { TriangleMeshGroup } from "./TriangleMeshGroup.js";
import { TriangleAccelerationStructure } from "../boundingVolumeHierarchy/TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "../boundingVolumeHierarchy/TriangleMeshAccelerationMethod.js";
// Installs `TriangleAccelerationStructure.create` for WALD2007_BINNED_SAH.
import "../boundingVolumeHierarchy/BinnedSahTriangleHierarchy.js";
import "../boundingVolumeHierarchy/GridTriangleHierarchy.js";

/**
This class represents a "basic" triangle mesh. Its model is based in a set
of vertexes and triangles (the edges are not store explicitly, and there
can not be an edge not forming part of a triangle).

This basic triangle mesh model can be associated with one and only one
material (this could change in future), but can have multiple textures,
and each texture can be mapped to a different set of triangles.

As every model class or `Entity` in VSDK, this class only can represent
(store in memory) the mesh model. It doesn't provide persistence or rendering
functionality, as this could be found at `io` and `render` packages.
Nevertheles, this class will be highly coupled with both of those, so
making any change here will impact highly that code.

This class does not ensure nor impose data integrity, and this will be the
sole responsability of the cooperating utilities and applications.

Java's `double[]` / `int[]` arrays are `Float64Array` / `Int32Array` here and
`ArrayList<ArrayList<Integer>>` is `number[][]`. Element accesses outside the
ray intersection loop go through `JavaArray`, so null arrays and out of range
indexes throw as in Java; the intersection itself (`doIntersectionInternal`,
`interpolateTriangleData`) indexes the arrays directly, as it only runs on
the triangles the acceleration structure or the brute force loop return.

\todo  Document more this class (include samples and data structure diagrams)
\todo  Generalize the material usage model, to conform similarly to current
      texture usage (i.e. allow multiple materials per mesh)
\todo  Make sure this is always using good names with complete words on it
      (rename methods and attributes)
\todo  Extend the model to allow dangling edges
*/
export class TriangleMesh extends Surface<Ray, RayHit> implements Cloneable {
    // Java `implements Cloneable`, as a run-time marker
    public static readonly [CLONEABLE] = true;

    //= Class attributes ========================================================

    // Basic mesh data model
    private name: string = "default";

    /**
     * Whether raster renderers must draw both windings of this surface.
     * MIT .ray scenes use two-sided triangle intersection, so their imported
     * meshes set this flag to keep the GPU result consistent with raytracing.
     */
    private doubleSided: boolean;

    // Consecutive triads of x/y/z point coordinates
    private vertexPositions: Float64Array | null;

    // Consecutive triads of nx/ny/nz vertex normal coordinates
    private vertexNormals: Float64Array | null;

    // Consecutive triads of nx/ny/nz vertex binormal coordinates (can be null)
    private vertexBinormals: Float64Array | null;

    // Consecutive triads of nx/ny/nz vertex tangent coordinates (can be null)
    private vertexTangents: Float64Array | null;

    // Consecutive triads of r/g/b vertex color components (can be null)
    private vertexColors: Float64Array | null;

    // Consecutive values of boolean. If true, then vertex is selected.
    private vertexSelections: boolean[] | null;

    // Consecutive pairs of u/v vertex texture coordinates (can be null)
    private vertexUvs: Float64Array | null;

    private incidentTrianglesPerVertexArray: number[][] | null;

    /// Consecutive triads of p0/p1/p2 indices into the vertex arrays
    private triangleIndexes: Int32Array | null;
    /// Consecutive triads of nx/ny/nz for normal vectors, one per triangle
    private triangleNormals: Float64Array | null;

    // Auxiliary components for data model
    private materials: SimpleMaterial[] | null = null;
    private textures: Image[] | null = null;

    /**
    textureRanges is a 2D array which contents mappings between the
    `triangles` and `textures` sets. Each pair
    <textureRanges[i][0], textureRanges[i][1]> means that the triangles
    from textureRanges[i-1][0] to textureRanges[i][0] (from triangle 0
    when i = 0), are associated with the texture textureRanges[i][1]-1
    (or no texture for unspecified range or when textureRanges[i][1]
    contains a value out of textures array bounds or a value of 0).
    */
    private textureRanges: number[][] | null = null;

    /**
    materialRanges is a 2D array which contents mappings between the
    `triangles` and `materials` sets. Each pair
    <materialRanges[i][0], materialRanges[i][1]> means that the triangles
    from materialRanges[i-1][0] to materialRanges[i][0] (from triangle 0
    when i = 0), are associated with the material materialRanges[i][1]
    (or no material for unspecified range or when materialRanges[i][1]
    contains a value out of materials array bounds).
    */
    private materialRanges: number[][] | null = null;

    // Auxiliary data structures for storage of parcial results and
    // preprocessing
    private readonly intersectionTriangleIndex: TriangleMeshIntersectionIndex = new TriangleMeshIntersectionIndex();
    // Optional ray acceleration structure, see prepareAccelerationStructure
    private accelerationStructure: TriangleAccelerationStructure | null = null;

    //= Basic class management methods ==========================================

    public constructor() {
        super();
        this.doubleSided = false;
        this.vertexPositions = null;
        this.vertexNormals = null;
        this.vertexBinormals = null;
        this.vertexTangents = null;
        this.vertexColors = null;
        this.vertexUvs = null;
        this.vertexSelections = null;
        this.triangleIndexes = null;
        this.triangleNormals = null;
        this.incidentTrianglesPerVertexArray = null;
    }

    public isDoubleSided(): boolean {
        return this.doubleSided;
    }

    public setDoubleSided(doubleSided: boolean): void {
        this.doubleSided = doubleSided;
    }

    private cloneDoubleArray(input: Float64Array | null): Float64Array | null {
        let out: Float64Array | null = null;

        if (input !== null) {
            out = new Float64Array(input.length);
            let i: number;
            for (i = 0; i < input.length; i++) {
                out[i] = input[i]!;
            }
        }

        return out;
    }

    private cloneBooleanArray(input: boolean[] | null): boolean[] | null {
        let out: boolean[] | null = null;

        if (input !== null) {
            out = new Array<boolean>(input.length);
            let i: number;
            for (i = 0; i < input.length; i++) {
                out[i] = input[i]!;
            }
        }

        return out;
    }

    private cloneIntArray(input: Int32Array | null): Int32Array | null {
        let out: Int32Array | null = null;

        if (input !== null) {
            out = new Int32Array(input.length);
            let i: number;
            for (i = 0; i < input.length; i++) {
                out[i] = input[i]!;
            }
        }

        return out;
    }

    /**
    \todo  copy full structure!
    @return a new TriangleMesh containing an exact copy of the original
    */
    public override clone(): TriangleMesh {
        super.clone();
        const other = new TriangleMesh();

        other.name = this.name;
        other.doubleSided = this.doubleSided;
        other.vertexPositions = this.cloneDoubleArray(this.vertexPositions);
        other.vertexPositions = this.cloneDoubleArray(this.vertexPositions);
        other.vertexNormals = this.cloneDoubleArray(this.vertexNormals);
        other.vertexBinormals = this.cloneDoubleArray(this.vertexBinormals);
        other.vertexTangents = this.cloneDoubleArray(this.vertexTangents);
        other.vertexColors = this.cloneDoubleArray(this.vertexColors);
        other.vertexUvs = this.cloneDoubleArray(this.vertexUvs);
        other.triangleNormals = this.cloneDoubleArray(this.triangleNormals);

        other.vertexSelections = this.cloneBooleanArray(this.vertexSelections);
        other.triangleIndexes = this.cloneIntArray(this.triangleIndexes);
        other.accelerationStructure = this.accelerationStructure !== null ? this.accelerationStructure.copy() : null;

        // Pending!
        other.incidentTrianglesPerVertexArray = null;
        other.materials = null;
        other.textures = null;
        other.textureRanges = null;
        other.materialRanges = null;

        return other;
    }

    /** Mathematical copy; scene-owned materials and textures are not copied. */
    public override copyGeometry(): Geometry | null {
        // cpp copy-constructs and clears materials, textures and both range
        // tables; Java clone() already leaves those unset.
        return this.clone();
    }

    public getVertexPositionsView(): Float64Array | null {
        return this.vertexPositions;
    }
    public getTriangleIndexesView(): Int32Array | null {
        return this.triangleIndexes;
    }
    public getTriangleNormalsView(): Float64Array | null {
        return this.triangleNormals;
    }

    public override getBounds(result: GeometryBounds): void {
        const triangleIndexes = this.triangleIndexes ?? new Int32Array(0);
        const vertexPositions = this.vertexPositions ?? new Float64Array(0);
        if (triangleIndexes.length % 3 !== 0 || vertexPositions.length % 3 !== 0) {
            result.setIndeterminate();
            return;
        }
        if (this.getNumTriangles() === 0) {
            result.setEmpty();
            return;
        }
        let minimum = new Vector3Dd(),
            maximum = new Vector3Dd();
        let first = true;
        for (let i = 0; i < triangleIndexes.length; ++i) {
            const index = triangleIndexes[i]!;
            if (index < 0 || index >= this.getNumVertices()) {
                result.setIndeterminate();
                return;
            }
            const x = vertexPositions[index * 3]!,
                y = vertexPositions[index * 3 + 1]!;
            const z = vertexPositions[index * 3 + 2]!;
            if (!Double.isFinite(x) || !Double.isFinite(y) || !Double.isFinite(z)) {
                result.setIndeterminate();
                return;
            }
            if (first) {
                minimum = maximum = new Vector3Dd(x, y, z);
                first = false;
            } else {
                minimum = new Vector3Dd(
                    JavaMath.min(minimum.x(), x),
                    JavaMath.min(minimum.y(), y),
                    JavaMath.min(minimum.z(), z),
                );
                maximum = new Vector3Dd(
                    JavaMath.max(maximum.x(), x),
                    JavaMath.max(maximum.y(), y),
                    JavaMath.max(maximum.z(), z),
                );
            }
        }
        if (first) result.setEmpty();
        else result.setFinite(minimum, maximum);
    }

    private getTrianglePoints(index: number, p0: Vector3Dd[], p1: Vector3Dd[], p2: Vector3Dd[]): boolean {
        const triangleIndexes = this.triangleIndexes ?? new Int32Array(0);
        const vertexPositions = this.vertexPositions ?? new Float64Array(0);
        if (
            index < 0 ||
            index >= this.getNumTriangles() ||
            vertexPositions.length % 3 !== 0 ||
            triangleIndexes.length % 3 !== 0
        )
            return false;
        const a = triangleIndexes[index * 3]!,
            b = triangleIndexes[index * 3 + 1]!,
            c = triangleIndexes[index * 3 + 2]!;
        if (
            a < 0 ||
            b < 0 ||
            c < 0 ||
            a >= this.getNumVertices() ||
            b >= this.getNumVertices() ||
            c >= this.getNumVertices()
        )
            return false;
        p0[0] = new Vector3Dd(vertexPositions[a * 3]!, vertexPositions[a * 3 + 1]!, vertexPositions[a * 3 + 2]!);
        p1[0] = new Vector3Dd(vertexPositions[b * 3]!, vertexPositions[b * 3 + 1]!, vertexPositions[b * 3 + 2]!);
        p2[0] = new Vector3Dd(vertexPositions[c * 3]!, vertexPositions[c * 3 + 1]!, vertexPositions[c * 3 + 2]!);
        return true;
    }

    private fillTypedDetail(
        ray: Ray,
        t: number,
        face: number,
        u: number,
        v: number,
        p0: Vector3Dd,
        p1: Vector3Dd,
        p2: Vector3Dd,
        normalReversed: boolean,
        hit: RayHit | null,
    ): void {
        if (!hit || !hit.needsAnySurfaceData()) return;
        hit.beginDetailEvaluation();
        if (hit.needsPoint()) {
            hit.point = ray.getOrigin().add(ray.getDirection().multiply(t));
            hit.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if (hit.needsNormal()) {
            const normal = p1.subtract(p0).crossProduct(p2.subtract(p0));
            const squared = normal.dotProduct(normal);
            if (Double.isFinite(squared) && squared > 0) {
                hit.normal =
                    squared < VSDK.EPSILON ? normal.multiply(1.0 / JavaMath.sqrt(squared)) : normal.normalized();
                if (normalReversed) hit.normal = hit.normal.multiply(-1);
                hit.markDetailAvailable(RayHit.DETAIL_NORMAL);
            }
        }
        const vertexUvs = this.vertexUvs ?? new Float64Array(0);
        if (hit.needsTextureCoordinates() && vertexUvs.length >= this.getNumVertices() * 2) {
            const triangleIndexes = this.triangleIndexes!;
            const a = triangleIndexes[face * 3]!,
                b = triangleIndexes[face * 3 + 1]!,
                c = triangleIndexes[face * 3 + 2]!;
            const w = 1.0 - u - v;
            const texU = w * vertexUvs[a * 2]! + u * vertexUvs[b * 2]! + v * vertexUvs[c * 2]!;
            const texV = w * vertexUvs[a * 2 + 1]! + u * vertexUvs[b * 2 + 1]! + v * vertexUvs[c * 2 + 1]!;
            if (Double.isFinite(texU) && Double.isFinite(texV)) {
                hit.u = texU;
                hit.v = texV;
                hit.markDetailAvailable(RayHit.DETAIL_UV);
            }
        }
    }

    public override queryFirstHit(
        ray: Ray,
        query: RayCrossingQuery,
        hit: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (hit) hit.clear();
        if (boundary) boundary.assign(new RayBoundary());
        if ((this.triangleIndexes?.length ?? 0) % 3 !== 0 || (this.vertexPositions?.length ?? 0) % 3 !== 0)
            return RayFirstHitStatus.Indeterminate;
        const t = [0],
            u = [0],
            v = [0];
        if (this.getNumTriangles() === 0) {
            const zero = new Vector3Dd();
            return Triangle.queryIntersection(ray, zero, zero, zero, query, t, u, v);
        }
        let best = -1;
        let bestT = 0,
            bestU = 0,
            bestV = 0;
        let bestP0 = new Vector3Dd(),
            bestP1 = new Vector3Dd(),
            bestP2 = new Vector3Dd();
        for (let i = 0; i < this.getNumTriangles(); ++i) {
            const p0 = [new Vector3Dd()],
                p1 = [new Vector3Dd()],
                p2 = [new Vector3Dd()];
            if (!this.getTrianglePoints(i, p0, p1, p2)) return RayFirstHitStatus.Indeterminate;
            const status = Triangle.queryIntersection(ray, p0[0]!, p1[0]!, p2[0]!, query, t, u, v);
            if (status !== RayFirstHitStatus.Hit && status !== RayFirstHitStatus.Miss) return status;
            if (status === RayFirstHitStatus.Hit && (best < 0 || t[0]! < bestT)) {
                best = i;
                bestT = t[0]!;
                bestU = u[0]!;
                bestV = v[0]!;
                bestP0 = p0[0]!;
                bestP1 = p1[0]!;
                bestP2 = p2[0]!;
            }
        }
        if (best < 0) return RayFirstHitStatus.Miss;
        const point = ray.getOrigin().add(ray.getDirection().multiply(bestT));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) || !Double.isFinite(point.z()))
            return RayFirstHitStatus.Indeterminate;
        if (boundary) boundary.assign(new RayBoundary(bestT, this, false, best));
        if (hit) {
            hit.setHitDistance(bestT);
            if (hit.shouldStoreRay()) hit.setRay(ray.withT(bestT));
            this.fillTypedDetail(ray, bestT, best, bestU, bestV, bestP0, bestP1, bestP2, false, hit);
        }
        return RayFirstHitStatus.Hit;
    }

    public override querySurfaceDetail(ray: Ray, boundary: RayBoundary, hit: RayHit | null): RayFirstHitStatus {
        if (hit) hit.clear();
        if (
            !hit ||
            boundary.geometry !== this ||
            boundary.surfaceIndex < 0 ||
            boundary.surfaceIndex >= this.getNumTriangles() ||
            !Double.isFinite(boundary.t)
        )
            return RayFirstHitStatus.InvalidInput;
        if (boundary.getInstanceCount() !== 0) return RayFirstHitStatus.Unsupported;
        const face = Math.trunc(boundary.surfaceIndex);
        const p0 = [new Vector3Dd()],
            p1 = [new Vector3Dd()],
            p2 = [new Vector3Dd()];
        if (!this.getTrianglePoints(face, p0, p1, p2)) return RayFirstHitStatus.Indeterminate;
        const t = [0],
            u = [0],
            v = [0];
        const status = Triangle.queryIntersection(
            ray,
            p0[0]!,
            p1[0]!,
            p2[0]!,
            new RayCrossingQuery(boundary.t, boundary.t),
            t,
            u,
            v,
        );
        if (status !== RayFirstHitStatus.Hit) return status;
        const point = ray.getOrigin().add(ray.getDirection().multiply(t[0]!));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) || !Double.isFinite(point.z()))
            return RayFirstHitStatus.Indeterminate;
        hit.setHitDistance(t[0]!);
        if (hit.shouldStoreRay()) hit.setRay(ray.withT(t[0]!));
        this.fillTypedDetail(ray, t[0]!, face, u[0]!, v[0]!, p0[0]!, p1[0]!, p2[0]!, boundary.normalReversed, hit);
        return RayFirstHitStatus.Hit;
    }

    public override emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        if ((this.triangleIndexes?.length ?? 0) % 3 !== 0 || (this.vertexPositions?.length ?? 0) % 3 !== 0)
            return RayCrossingSupport.Indeterminate;
        const depths = new ArrayList<number>();
        const faces = new ArrayList<number>();
        const t = [0],
            u = [0],
            v = [0];
        if (this.getNumTriangles() === 0) {
            const zero = new Vector3Dd();
            const status = Triangle.queryIntersection(ray, zero, zero, zero, query, t, u, v);
            if (status === RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
            if (status === RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
            if (status === RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
            return RayCrossingSupport.Supported;
        }
        for (let i = 0; i < this.getNumTriangles(); ++i) {
            const p0 = [new Vector3Dd()],
                p1 = [new Vector3Dd()],
                p2 = [new Vector3Dd()];
            if (!this.getTrianglePoints(i, p0, p1, p2)) return RayCrossingSupport.Indeterminate;
            const status = Triangle.queryIntersection(ray, p0[0]!, p1[0]!, p2[0]!, query, t, u, v);
            if (status === RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
            if (status === RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
            if (status === RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
            if (status === RayFirstHitStatus.Hit) {
                const point = ray.getOrigin().add(ray.getDirection().multiply(t[0]!));
                if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) || !Double.isFinite(point.z()))
                    return RayCrossingSupport.Indeterminate;
                let at = 0;
                while (at < depths.size() && depths.get(at) <= t[0]!) ++at;
                if (at === depths.size()) {
                    depths.add(t[0]!);
                    faces.add(i);
                } else {
                    depths.add(at, t[0]!);
                    faces.add(at, i);
                }
            }
        }
        for (let i = 0; i < depths.size(); ++i) {
            const event = new RayCrossing(depths.get(i), this, RayCrossingKind.Unclassified);
            event.surfaceIndex = faces.get(i);
            sink.crossing(event);
        }
        return RayCrossingSupport.Supported;
    }

    public getName(): string {
        return this.name;
    }

    public getVertexAt(i: number, vertex: Vertex): void {
        vertex.position = vertex.position.withX(JavaArray.get(this.vertexPositions, 3 * i));
        vertex.position = vertex.position.withY(JavaArray.get(this.vertexPositions, 3 * i + 1));
        vertex.position = vertex.position.withZ(JavaArray.get(this.vertexPositions, 3 * i + 2));
        if (this.vertexNormals !== null) {
            vertex.normal = vertex.normal.withX(JavaArray.get(this.vertexNormals, 3 * i));
            vertex.normal = vertex.normal.withY(JavaArray.get(this.vertexNormals, 3 * i + 1));
            vertex.normal = vertex.normal.withZ(JavaArray.get(this.vertexNormals, 3 * i + 2));
        }
        if (this.vertexBinormals !== null) {
            vertex.binormal = vertex.binormal.withX(JavaArray.get(this.vertexBinormals, 3 * i));
            vertex.binormal = vertex.binormal.withY(JavaArray.get(this.vertexBinormals, 3 * i + 1));
            vertex.binormal = vertex.binormal.withZ(JavaArray.get(this.vertexBinormals, 3 * i + 2));
        }
        if (this.vertexTangents !== null) {
            vertex.tangent = vertex.tangent.withX(JavaArray.get(this.vertexTangents, 3 * i));
            vertex.tangent = vertex.tangent.withY(JavaArray.get(this.vertexTangents, 3 * i + 1));
            vertex.tangent = vertex.tangent.withZ(JavaArray.get(this.vertexTangents, 3 * i + 2));
        }
        if (this.vertexUvs !== null) {
            vertex.u = JavaArray.get(this.vertexUvs, 2 * i);
            vertex.v = JavaArray.get(this.vertexUvs, 2 * i + 1);
        }
    }

    public getMaterials(): SimpleMaterial[] | null {
        return this.materials;
    }

    public getTextures(): Image[] | null {
        return this.textures;
    }

    public getTextureAt(index: number): Image | null {
        return JavaArray.get(this.textures, index);
    }

    public setName(name: string): void {
        this.name = name;
    }

    public initVertexPositionsArray(n: number): void {
        this.accelerationStructure = null;
        this.vertexPositions = new Float64Array(n * 3);
    }

    public initVertexNormalsArray(): void {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        const n = Math.trunc(positions.length / 3);
        if (this.vertexNormals === null || this.vertexNormals.length !== positions.length) {
            this.vertexNormals = new Float64Array(n * 3);
        }
    }

    public initVertexBinormalsArray(): void {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        const n = Math.trunc(positions.length / 3);
        if (this.vertexBinormals === null || this.vertexBinormals.length !== positions.length) {
            this.vertexBinormals = new Float64Array(n * 3);
        }
    }

    public initVertexTangentsArray(): void {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        const n = Math.trunc(positions.length / 3);
        if (this.vertexTangents === null || this.vertexTangents.length !== positions.length) {
            this.vertexTangents = new Float64Array(n * 3);
        }
    }

    public initVertexColorsArray(): void {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        const n = Math.trunc(positions.length / 3);
        if (this.vertexColors === null || this.vertexColors.length !== positions.length) {
            this.vertexColors = new Float64Array(n * 3);
        }
    }

    public initVertexUvsArray(): void {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        const n = Math.trunc(positions.length / 3);
        if (this.vertexUvs === null || Math.trunc(this.vertexUvs.length / 2) !== Math.trunc(positions.length / 3)) {
            this.vertexUvs = new Float64Array(n * 2);
        }
    }

    public initIncidentTrianglesPerVertexArray(): void {
        this.incidentTrianglesPerVertexArray = [];
    }

    public detachColors(): void {
        this.vertexColors = null;
    }

    public detachNormals(): void {
        this.vertexNormals = null;
    }

    public detachUvs(): void {
        this.vertexUvs = null;
    }

    /**
    This method provides a clear structured form of defining the mesh vertexes,
    but it is inefficient. Its use is discouraged for applications manipulating
    big meshes.
    */
    public setVertexes(
        vertexes: Vertex[],
        withNormals: boolean,
        withBinormals: boolean,
        withTangents: boolean,
        withUvs: boolean,
    ): void {
        let i: number;

        const n = vertexes.length;

        this.initVertexPositionsArray(n);
        if (withNormals) {
            this.initVertexNormalsArray();
        }
        if (withBinormals) {
            this.initVertexBinormalsArray();
        }
        if (withTangents) {
            this.initVertexTangentsArray();
        }
        if (withUvs) {
            this.initVertexUvsArray();
        }

        const vertexPositions = this.vertexPositions!;
        for (i = 0; i < n; i++) {
            const vertex = TriangleMesh.nonNull(vertexes[i] ?? null);
            vertexPositions[3 * i] = vertex.position.x();
            vertexPositions[3 * i + 1] = vertex.position.y();
            vertexPositions[3 * i + 2] = vertex.position.z();
            if (this.vertexNormals !== null) {
                this.vertexNormals[3 * i] = vertex.normal.x();
                this.vertexNormals[3 * i + 1] = vertex.normal.y();
                this.vertexNormals[3 * i + 2] = vertex.normal.z();
            }
            if (this.vertexBinormals !== null) {
                this.vertexBinormals[3 * i] = vertex.binormal.x();
                this.vertexBinormals[3 * i + 1] = vertex.binormal.y();
                this.vertexBinormals[3 * i + 2] = vertex.binormal.z();
            }
            if (this.vertexTangents !== null) {
                this.vertexTangents[3 * i] = vertex.tangent.x();
                this.vertexTangents[3 * i + 1] = vertex.tangent.y();
                this.vertexTangents[3 * i + 2] = vertex.tangent.z();
            }
            if (this.vertexUvs !== null) {
                this.vertexUvs[2 * i] = vertex.u;
                this.vertexUvs[2 * i + 1] = vertex.v;
            }
        }
    }

    public initTriangleArrays(n: number): void {
        this.accelerationStructure = null;
        this.triangleIndexes = new Int32Array(n * 3);
        this.triangleNormals = new Float64Array(n * 3);
    }

    /**
    This method provides a clear structured form of defining the mesh triangles,
    but it is inefficient. Its use is discouraged for applications manipulating
    big meshes.
    */
    public setTriangles(triangles: Triangle[]): void {
        let i: number;

        const n = triangles.length;

        this.initTriangleArrays(n);

        const triangleIndexes = this.triangleIndexes!;
        for (i = 0; i < n; i++) {
            const triangle = TriangleMesh.nonNull(triangles[i] ?? null);
            triangleIndexes[3 * i] = triangle.getPoint0();
            triangleIndexes[3 * i + 1] = triangle.getPoint1();
            triangleIndexes[3 * i + 2] = triangle.getPoint2();
            if (this.triangleNormals !== null) {
                this.triangleNormals[3 * i] = triangle.getNormal().x();
                this.triangleNormals[3 * i + 1] = triangle.getNormal().y();
                this.triangleNormals[3 * i + 2] = triangle.getNormal().z();
            }
        }
    }

    public setTextures(textures: Image[] | null): void {
        this.textures = textures;
    }

    public setMaterials(materials: SimpleMaterial[] | null): void {
        this.materials = materials;
    }

    /**
    Given a vertex structure and an `i` position, this method copies
    the information from the structure in to the i-th vertex arrays position.
    PRE: 0 <= i < vertexPositions.length/3
    */
    public setVertexAt(i: number, vertex: Vertex): void {
        this.accelerationStructure = null;
        JavaArray.set(this.vertexPositions, 3 * i, vertex.position.x());
        JavaArray.set(this.vertexPositions, 3 * i + 1, vertex.position.y());
        JavaArray.set(this.vertexPositions, 3 * i + 2, vertex.position.z());
        JavaArray.set(this.vertexNormals, 3 * i, vertex.normal.x());
        JavaArray.set(this.vertexNormals, 3 * i + 1, vertex.normal.y());
        JavaArray.set(this.vertexNormals, 3 * i + 2, vertex.normal.z());
        JavaArray.set(this.vertexBinormals, 3 * i, vertex.binormal.x());
        JavaArray.set(this.vertexBinormals, 3 * i + 1, vertex.binormal.y());
        JavaArray.set(this.vertexBinormals, 3 * i + 2, vertex.binormal.z());
        JavaArray.set(this.vertexTangents, 3 * i, vertex.tangent.x());
        JavaArray.set(this.vertexTangents, 3 * i + 1, vertex.tangent.y());
        JavaArray.set(this.vertexTangents, 3 * i + 2, vertex.tangent.z());
        JavaArray.set(this.vertexUvs, 2 * i, vertex.u);
        JavaArray.set(this.vertexUvs, 2 * i + 1, vertex.v);
    }

    /**
    Given a triangle structure and an `i` position, this method copies
    the information from the structure in to the i-th triangle arrays position.
    PRE: 0 <= i < vertexPositions.length/3
    */
    public setTriangleAt(i: number, triangle: Triangle): void {
        this.accelerationStructure = null;
        JavaArray.set(this.triangleIndexes, 3 * i, triangle.getPoint0());
        JavaArray.set(this.triangleIndexes, 3 * i + 1, triangle.getPoint1());
        JavaArray.set(this.triangleIndexes, 3 * i + 2, triangle.getPoint2());
        if (this.triangleNormals !== null) {
            JavaArray.set(this.triangleNormals, 3 * i, triangle.getNormal().x());
            JavaArray.set(this.triangleNormals, 3 * i + 1, triangle.getNormal().y());
            JavaArray.set(this.triangleNormals, 3 * i + 2, triangle.getNormal().z());
        }
    }

    public getNumVertices(): number {
        if (this.vertexPositions === null) return 0;
        return Math.trunc(this.vertexPositions.length / 3);
    }

    public getNumTriangles(): number {
        if (this.triangleIndexes === null) return 0;
        return Math.trunc(this.triangleIndexes.length / 3);
    }

    public getVertexSelections(): boolean[] {
        const positions = TriangleMesh.nonNull(this.vertexPositions);
        if (this.vertexSelections === null || this.vertexSelections.length !== Math.trunc(positions.length / 3)) {
            this.vertexSelections = new Array<boolean>(Math.trunc(positions.length / 3));
            let i: number;
            for (i = 0; i < this.vertexSelections.length; i++) {
                this.vertexSelections[i] = false;
            }
        }
        return this.vertexSelections;
    }

    public getVertexPositions(): Float64Array | null {
        return this.vertexPositions;
    }

    public getVertexNormals(): Float64Array | null {
        return this.vertexNormals;
    }

    public getVertexBinormals(): Float64Array | null {
        return this.vertexBinormals;
    }

    public getVertexTangents(): Float64Array | null {
        return this.vertexTangents;
    }

    public getVertexColors(): Float64Array | null {
        return this.vertexColors;
    }

    public getVertexUvs(): Float64Array | null {
        return this.vertexUvs;
    }

    public getTriangleIndexes(): Int32Array | null {
        return this.triangleIndexes;
    }

    public getTriangleNormals(): Float64Array | null {
        return this.triangleNormals;
    }

    public setTextureAt(index: number, image: Image): void {
        JavaArray.set(this.textures, index, image);
    }

    //= Methods for managing textureRanges ======================================

    public getTextureRanges(): number[][] | null {
        return this.textureRanges;
    }

    /**
    Note this always returns an array with two (2) integers: the first one
    is an index to `triangles` array, the second one is an index to the
    `textures` array.
    @return an integer array for textures ranges indexes
    */
    public getTextureRangeAt(spanRange: number): number[] {
        return JavaArray.get(this.textureRanges, spanRange);
    }

    public setTextureRanges(ranges: number[][] | null): void {
        this.textureRanges = ranges;
    }

    //= Methods for managing materialRanges =====================================

    public getMaterialRanges(): number[][] | null {
        return this.materialRanges;
    }

    /**
    Note this always returns an array with two (2) integers: the first one
    is an index to `triangles` array, the second one is an index to the
    `materials` array.
    @return a integer array with material ranges indexes
    */
    public getMaterialRangeAt(spanRange: number): number[] {
        return JavaArray.get(this.materialRanges, spanRange);
    }

    public setMaterialRanges(ranges: number[][] | null): void {
        this.materialRanges = ranges;
    }

    //= Fundamental geometry operations methods =================================

    /** `vertexPositions[3*triangleIndexes[3*i+k]+c]` with Java's checks. */
    private trianglePosition(i: number, k: number): Vector3Dd {
        const index = JavaArray.get(this.triangleIndexes, 3 * i + k);
        return new Vector3Dd(
            JavaArray.get(this.vertexPositions, 3 * index + 0),
            JavaArray.get(this.vertexPositions, 3 * index + 1),
            JavaArray.get(this.vertexPositions, 3 * index + 2),
        );
    }

    /** `vertexNormals[3*triangleIndexes[3*i+k]+c]` with Java's checks. */
    private triangleVertexNormal(i: number, k: number): Vector3Dd {
        const index = JavaArray.get(this.triangleIndexes, 3 * i + k);
        return new Vector3Dd(
            JavaArray.get(this.vertexNormals, 3 * index + 0),
            JavaArray.get(this.vertexNormals, 3 * index + 1),
            JavaArray.get(this.vertexNormals, 3 * index + 2),
        );
    }

    /** `ArrayList.get` over the incidence lists. */
    private static listGet<T>(list: T[], index: number): T {
        if (index < 0 || index >= list.length) {
            throw new IndexOutOfBoundsException("Index " + index + " out of bounds for length " + list.length);
        }
        return list[index]!;
    }

    public calculateNormals(): void {
        let tn: Vector3Dd;
        let i: number;
        let j: number;

        let v1: Vector3Dd;
        let v2: Vector3Dd;
        let v3: Vector3Dd;

        for (i = 0; i < this.getNumTriangles(); i++) {
            v1 = this.trianglePosition(i, 0);
            v2 = this.trianglePosition(i, 1);
            v3 = this.trianglePosition(i, 2);

            const ax = v2.x() - v1.x();
            const ay = v2.y() - v1.y();
            const az = v2.z() - v1.z();

            const bx = v3.x() - v2.x();
            const by = v3.y() - v2.y();
            const bz = v3.z() - v2.z();

            let a = new Vector3Dd(ax, ay, az);
            let b = new Vector3Dd(bx, by, bz);

            a = a.normalized();
            b = b.normalized();

            tn = a.crossProduct(b);
            tn = tn.normalized();
            JavaArray.set(this.triangleNormals, 3 * i, tn.x());
            JavaArray.set(this.triangleNormals, 3 * i + 1, tn.y());
            JavaArray.set(this.triangleNormals, 3 * i + 2, tn.z());
        }

        this.initIncidentTrianglesPerVertexArray();
        const incident = this.incidentTrianglesPerVertexArray!;
        for (i = 0; i < this.getNumVertices(); i++) {
            incident.push([]);
        }

        for (i = 0; i < this.getNumTriangles(); i++) {
            TriangleMesh.listGet(incident, JavaArray.get(this.triangleIndexes, 3 * i + 0)).push(i);
            TriangleMesh.listGet(incident, JavaArray.get(this.triangleIndexes, 3 * i + 1)).push(i);
            TriangleMesh.listGet(incident, JavaArray.get(this.triangleIndexes, 3 * i + 2)).push(i);
        }

        this.initVertexNormalsArray();

        for (i = 0; i < this.getNumVertices(); i++) {
            let nx = 0.0;
            let ny = 0.0;
            let nz = 0.0;
            const list = TriangleMesh.listGet(incident, i);
            for (j = 0; j < list.length; j++) {
                const ii = list[j]!;
                nx += this.triangleNormals![3 * ii + 0]!;
                ny += this.triangleNormals![3 * ii + 1]!;
                nz += this.triangleNormals![3 * ii + 2]!;
            }
            const n = new Vector3Dd(nx, ny, nz).normalized();
            this.vertexNormals![3 * i] = n.x();
            this.vertexNormals![3 * i + 1] = n.y();
            this.vertexNormals![3 * i + 2] = n.z();
        }
    }

    /**
    In the triangle meshes the vector normal directions must be consistent with
    the triangle normal, which direction depends on triangle order (clockwise
    or counterclockwise).

    This method checks for inverted normals, and invert them to make it
    consistent with triangle orientation order.

    Note that this methods recalculate triangle surface normals.
    */
    public reorientateNormals(): void {
        let i: number;
        let a: Vector3Dd;
        let b: Vector3Dd;
        let c: Vector3Dd;
        let u: Vector3Dd;
        let v: Vector3Dd;
        let n: Vector3Dd;
        let an: Vector3Dd;
        let bn: Vector3Dd;
        let cn: Vector3Dd;

        for (i = 0; i < this.getNumTriangles(); i++) {
            a = this.trianglePosition(i, 0);
            b = this.trianglePosition(i, 1);
            c = this.trianglePosition(i, 2);

            an = this.triangleVertexNormal(i, 0);
            bn = this.triangleVertexNormal(i, 1);
            cn = this.triangleVertexNormal(i, 2);

            u = b.subtract(a);
            v = c.subtract(a);
            n = u.crossProduct(v);
            n = n.normalized();
            if (this.triangleNormals !== null) {
                JavaArray.set(this.triangleNormals, 3 * i + 0, n.x());
                JavaArray.set(this.triangleNormals, 3 * i + 1, n.y());
                JavaArray.set(this.triangleNormals, 3 * i + 2, n.z());
            }
            const ia = this.triangleIndexes![3 * i]!;
            const ib = this.triangleIndexes![3 * i + 1]!;
            const ic = this.triangleIndexes![3 * i + 2]!;
            if (n.dotProduct(an) < 0) {
                an = an.multiply(-1);
                this.vertexNormals![3 * ia + 0] = an.x();
                this.vertexNormals![3 * ia + 1] = an.y();
                this.vertexNormals![3 * ia + 2] = an.z();
            }
            if (n.dotProduct(bn) < 0) {
                bn = bn.multiply(-1);
                this.vertexNormals![3 * ib + 0] = bn.x();
                this.vertexNormals![3 * ib + 1] = bn.y();
                this.vertexNormals![3 * ib + 2] = bn.z();
            }
            if (n.dotProduct(cn) < 0) {
                cn = cn.multiply(-1);
                this.vertexNormals![3 * ic + 0] = cn.x();
                this.vertexNormals![3 * ic + 1] = cn.y();
                this.vertexNormals![3 * ic + 2] = cn.z();
            }
        }
    }

    /** Needed for supplying the Geometry.getMinMax operation */
    private calculateMinMaxPositions(): number[] {
        const minMax: number[] = [0, 0, 0, 0, 0, 0];

        let minX = Double.MAX_VALUE;
        let minY = Double.MAX_VALUE;
        let minZ = Double.MAX_VALUE;
        let maxX = -Double.MAX_VALUE;
        let maxY = -Double.MAX_VALUE;
        let maxZ = -Double.MAX_VALUE;
        let i: number;

        const vertexPositions = this.vertexPositions;
        for (i = 0; i < this.getNumVertices(); i++) {
            const x = vertexPositions![3 * i + 0]!;
            const y = vertexPositions![3 * i + 1]!;
            const z = vertexPositions![3 * i + 2]!;

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
    This method is supposed to be a friend of TriangleMesh related objects.
    The method is used to query the last intersected triangle, after a
    positive called to the doIntersectionFirstHit method.
    @return selected triangle
    */
    public doIntersectionInformation(): number {
        return this.intersectionTriangleIndex.get();
    }

    /**
    Makes `doIntersectionFirstHit` use the given method: builds its
    acceleration structure (single threaded) unless one of the same method
    is already built for the current sizes; `BRUTE_FORCE` releases it. Call
    it before tracing from several threads: afterwards, calls with the same
    method only read. The result of the intersection does not depend on the
    method.

    The structure is a snapshot: the mutators of this class release it, but
    writes through the array getters (`getVertexPositions`,
    `getTriangleIndexes`) do not, so call `releaseAccelerationStructure` or
    this method again after them.
    @param method acceleration method to use
    */
    public prepareAccelerationStructure(method: TriangleMeshAccelerationMethod): void {
        if (method === TriangleMeshAccelerationMethod.BRUTE_FORCE) {
            if (this.accelerationStructure !== null) this.accelerationStructure = null;
            return;
        }
        const current = this.accelerationStructure;
        if (
            current !== null &&
            current.getMethod() === method &&
            current.matches(this.getNumVertices(), this.getNumTriangles())
        )
            return;
        const structure = TriangleAccelerationStructure.create(method);
        if (structure !== null)
            structure.build(TriangleMesh.nonNull(this.vertexPositions), TriangleMesh.nonNull(this.triangleIndexes));
        this.accelerationStructure = structure;
    }

    public releaseAccelerationStructure(): void {
        this.accelerationStructure = null;
    }

    /**
    @return current structure, null when the brute force loop is used
    */
    public getAccelerationStructure(): TriangleAccelerationStructure | null {
        return this.accelerationStructure;
    }

    /**
    When a structure built for the current sizes exists: writes what
    `getMinMax` would return, without walking the vertices, and returns true.
    @param outMinMax six values array to fill
    @return true when `outMinMax` was written
    */
    public getPreparedMinMax(outMinMax: number[] | Float64Array): boolean {
        const structure = this.accelerationStructure;
        if (structure === null || !structure.matches(this.getNumVertices(), this.getNumTriangles())) return false;
        structure.getVertexBounds(outMinMax);
        return true;
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable representation of current TriangleMesh information
    */
    public override toString(): string {
        let msg: string;
        let i: number;

        const mm = this.getMinMax();
        let p: Vector3Dd;

        msg = "- TriangleMesh ------------------------------------------------------------\n";
        msg += "  - Number of triangles:" + this.getNumTriangles() + "\n";
        msg += "  - Number of vertexes:" + this.getNumVertices() + "\n";
        p = new Vector3Dd(mm[0]!, mm[1]!, mm[2]!);
        msg += "  - MINMAX: " + p.toString();
        p = new Vector3Dd(mm[3]!, mm[4]!, mm[5]!);
        msg += " - " + p.toString() + "\n";
        if (this.materials === null) {
            msg += "  - No materials available!\n";
        } else {
            msg += "  - " + this.materials.length + " materials\n";
        }

        if (this.materialRanges === null) {
            msg += "  - No material ranges association table available!\n";
        } else {
            msg += "  - " + this.materialRanges.length + " material spans:\n";
            for (i = 0; i < this.materialRanges.length; i++) {
                const range = TriangleMesh.nonNull(this.materialRanges[i] ?? null);
                msg += "    . " + JavaArray.get(range, 0) + " -> " + JavaArray.get(range, 1) + "\n";
            }
        }

        if (this.textures === null) {
            msg += "  - No textures available!\n";
        } else {
            msg += "  - " + this.textures.length + " textures\n";
        }

        if (this.textureRanges === null) {
            msg += "  - No texture ranges association table available!\n";
        } else {
            msg += "  - " + this.textureRanges.length + " texture spans:\n";
            for (i = 0; i < this.textureRanges.length; i++) {
                const range = TriangleMesh.nonNull(this.textureRanges[i] ?? null);
                msg += "    . " + JavaArray.get(range, 0) + " -> " + JavaArray.get(range, 1) + "\n";
            }
        }

        msg += "---------------------------------------------------------------------------\n";
        return msg;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit. Java overloads
    `doIntersectionFirstHit(Ray)`, `doIntersectionFirstHit(Ray, RayHit)` and
    the package private `doIntersectionFirstHit(Ray, RayHit, int[])`.
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit | null): boolean;
    public doIntersectionFirstHit(
        inRay: Ray,
        outHit: RayHit | null,
        outTriangleIndex: number[] | Int32Array | null,
    ): boolean;
    public override doIntersectionFirstHit(
        inRay: Ray,
        outHit?: RayHit | null,
        outTriangleIndex?: number[] | Int32Array | null,
    ): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit(RayHit.DETAIL_NONE, true);
            if (this.doIntersectionFirstHit(inRay, hit)) {
                return hit.getRay();
            }
            return null;
        }
        return this.doIntersectionInternal(inRay, outHit, outTriangleIndex ?? null);
    }

    private doIntersectionInternal(
        inRay: Ray,
        outHit: RayHit | null,
        outTriangleIndex: number[] | Int32Array | null,
    ): boolean {
        const ray = new Ray(inRay);
        const bestT: number[] = [1e308];
        const bestTri: number[] = [-1];
        const bestU: number[] = [0];
        const bestV: number[] = [0];
        let found: boolean;

        const vertexPositions = this.vertexPositions;
        const triangleIndexes = this.triangleIndexes;
        const structure = this.accelerationStructure;
        if (structure !== null && structure.matches(this.getNumVertices(), this.getNumTriangles())) {
            found = structure.firstHit(ray, vertexPositions!, triangleIndexes!, bestT, bestU, bestV, bestTri);
        } else {
            found = TriangleAccelerationStructure.firstHitBruteForce(
                ray,
                TriangleMesh.nonNull(vertexPositions),
                TriangleMesh.nonNull(triangleIndexes),
                bestT,
                bestU,
                bestV,
                bestTri,
            );
        }
        const bestTriangle = bestTri[0]!;
        let bestHit: Intersection | null = null;
        if (found) {
            // Same point and geometric normal of
            // Triangle.doIntersectionWithTriangle, used before by this loop.
            const p = vertexPositions!;
            const t = triangleIndexes!;
            const v0 = new Vector3Dd(
                p[3 * t[3 * bestTriangle + 0]! + 0]!,
                p[3 * t[3 * bestTriangle + 0]! + 1]!,
                p[3 * t[3 * bestTriangle + 0]! + 2]!,
            );
            const v1 = new Vector3Dd(
                p[3 * t[3 * bestTriangle + 1]! + 0]!,
                p[3 * t[3 * bestTriangle + 1]! + 1]!,
                p[3 * t[3 * bestTriangle + 1]! + 2]!,
            );
            const v2 = new Vector3Dd(
                p[3 * t[3 * bestTriangle + 2]! + 0]!,
                p[3 * t[3 * bestTriangle + 2]! + 1]!,
                p[3 * t[3 * bestTriangle + 2]! + 2]!,
            );
            bestHit = new Intersection(
                bestT[0]!,
                ray.getOrigin().add(ray.getDirection().multiply(bestT[0]!)),
                v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized(),
            );
        }

        if (bestHit === null) {
            this.intersectionTriangleIndex.set(-1);
            if (outTriangleIndex !== null && outTriangleIndex.length > 0) {
                outTriangleIndex[0] = -1;
            }
            return false;
        }
        this.intersectionTriangleIndex.set(bestTriangle);
        if (outTriangleIndex !== null && outTriangleIndex.length > 0) {
            outTriangleIndex[0] = bestTriangle;
        }

        if (outHit !== null) {
            outHit.setRay(inRay.withT(bestHit.getT()));
            outHit.point = new Vector3Dd(bestHit.getPoint());
            outHit.normal = new Vector3Dd(bestHit.getNormal()).normalized();
            outHit.tangent = new Vector3Dd();
            outHit.u = 0;
            outHit.v = 0;

            this.interpolateTriangleData(bestTriangle, outHit, inRay.getDirection());
            this.fillMaterialAndTexture(bestTriangle, outHit);
        }
        return true;
    }

    private interpolateTriangleData(triangleIndex: number, outHit: RayHit, rayDirection: Vector3Dd): void {
        if (this.vertexNormals === null || this.vertexUvs === null) {
            if (outHit.normal.dotProduct(rayDirection) >= 0) {
                outHit.normal = outHit.normal.multiply(-1);
            }
            return;
        }

        const vp = this.vertexPositions!;
        const ti = this.triangleIndexes!;
        const vn = this.vertexNormals;
        const uv = this.vertexUvs;
        const i0 = ti[3 * triangleIndex + 0]!;
        const i1 = ti[3 * triangleIndex + 1]!;
        const i2 = ti[3 * triangleIndex + 2]!;

        const p0 = new Vector3Dd(vp[3 * i0 + 0]!, vp[3 * i0 + 1]!, vp[3 * i0 + 2]!);
        const p1 = new Vector3Dd(vp[3 * i1 + 0]!, vp[3 * i1 + 1]!, vp[3 * i1 + 2]!);
        const p2 = new Vector3Dd(vp[3 * i2 + 0]!, vp[3 * i2 + 1]!, vp[3 * i2 + 2]!);

        const A = p0.x() - p2.x();
        const B = p1.x() - p2.x();
        const C = p2.x() - outHit.point.x();
        const D = p0.y() - p2.y();
        const E = p1.y() - p2.y();
        const F = p2.y() - outHit.point.y();
        const G = p0.z() - p2.z();
        const H = p1.z() - p2.z();
        const I = p2.z() - outHit.point.z();

        const lambda0 = (B * (F + I) - C * (E + H)) / (A * (E + H) - B * (D + G));
        const lambda1 = (A * (F + I) - C * (D + G)) / (B * (D + G) - A * (E + H));
        const lambda2 = 1 - lambda0 - lambda1;

        const n0 = new Vector3Dd(vn[3 * i0 + 0]!, vn[3 * i0 + 1]!, vn[3 * i0 + 2]!);
        const n1 = new Vector3Dd(vn[3 * i1 + 0]!, vn[3 * i1 + 1]!, vn[3 * i1 + 2]!);
        const n2 = new Vector3Dd(vn[3 * i2 + 0]!, vn[3 * i2 + 1]!, vn[3 * i2 + 2]!);
        outHit.normal = n0
            .multiply(lambda0)
            .add(n1.multiply(lambda1).add(n2.multiply(lambda2)))
            .normalized();

        const u0 = uv[2 * i0 + 0]!;
        const v0 = uv[2 * i0 + 1]!;
        const u1 = uv[2 * i1 + 0]!;
        const v1 = uv[2 * i1 + 1]!;
        const u2 = uv[2 * i2 + 0]!;
        const v2 = uv[2 * i2 + 1]!;
        outHit.u = u0 * lambda0 + u1 * lambda1 + u2 * lambda2;
        outHit.v = v0 * lambda0 + v1 * lambda1 + v2 * lambda2;

        if (outHit.normal.dotProduct(rayDirection) >= 0) {
            outHit.normal = outHit.normal.multiply(-1);
        }
    }

    private fillMaterialAndTexture(triangleIndex: number, outHit: RayHit): void {
        if (this.materials !== null) {
            outHit.material = JavaArray.get(this.materials, 0);
        }
        if (this.materialRanges !== null) {
            for (let i = 0; i < this.materialRanges.length - 1; i++) {
                const range = TriangleMesh.nonNull(this.materialRanges[i] ?? null);
                const next = TriangleMesh.nonNull(this.materialRanges[i + 1] ?? null);
                if (triangleIndex >= JavaArray.get(range, 0) && triangleIndex < JavaArray.get(next, 0)) {
                    outHit.material = JavaArray.get(this.materials, JavaArray.get(next, 1));
                    break;
                }
            }
        }

        outHit.texture = null;
        if (this.textureRanges !== null) {
            for (let i = 0; i < this.textureRanges.length - 1; i++) {
                const range = TriangleMesh.nonNull(this.textureRanges[i] ?? null);
                const next = TriangleMesh.nonNull(this.textureRanges[i + 1] ?? null);
                if (triangleIndex >= JavaArray.get(range, 0) && triangleIndex < JavaArray.get(next, 0)) {
                    outHit.texture = JavaArray.get(this.textures, JavaArray.get(next, 1) - 1);
                    break;
                }
            }
        }
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if ((outData as RayHit | null) === null) {
            return;
        }
        const queryRay = inRay.withT(inT);
        this.doIntersectionFirstHit(queryRay, outData);
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    \todo  Check efficiency for this implementation. Note that for the
    special application of volume rendering generation, it is better
    to provide another method, to add voxels after a path following
    over the line.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        let i: number;
        let status: number;
        let p0: Vector3Dd;
        let p1: Vector3Dd;
        let p2: Vector3Dd;

        for (i = 0; i < this.getNumTriangles(); i++) {
            p0 = this.trianglePosition(i, 0);
            p1 = this.trianglePosition(i, 1);
            p2 = this.trianglePosition(i, 2);
            status = Triangle.containmentTest(p0, p1, p2, p, distanceTolerance);
            if (status !== TriangleMesh.OUTSIDE) {
                return TriangleMesh.LIMIT;
            }
        }

        return TriangleMesh.OUTSIDE;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.getMinMax.
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): number[] {
        return this.calculateMinMaxPositions();
    }

    public exportToTriangleMeshGroup(): TriangleMeshGroup {
        const group = new TriangleMeshGroup();
        group.addMesh(this);
        return group;
    }

    /**
    This method does the following:
    1. Detect all triangles with "incorrect" vertex indices and removes them.
    2. Detect all vertices without incident triangles.
    3. Reacomodate mesh representation arrays to fit the new situation. This
       can imply that some triangle indices gets updated.
    */
    public compact(): void {
        this.accelerationStructure = null;
        this.vertexNormals = null;

        //- Count triangle incidences on vertices -------------------------
        let i: number;
        let a: number;
        let b: number;
        let c: number;

        const n = Math.trunc(TriangleMesh.nonNull(this.vertexPositions).length / 3);
        const count: boolean[] = new Array<boolean>(n);

        for (i = 0; i < n; i++) {
            count[i] = false;
        }

        const triangleIndexes = TriangleMesh.nonNull(this.triangleIndexes);
        for (i = 0; i < triangleIndexes.length; i++) {
            a = triangleIndexes[i]!;
            if (a >= 0 && a < n) {
                count[a] = true;
            }
        }

        //- Calculate new index map ---------------------------------------
        let j = 0;
        const map = new Int32Array(n);
        for (i = 0; i < n; i++) {
            if (count[i]) {
                map[i] = j;
                j++;
            } else {
                map[i] = -1;
            }
        }

        //- Calculate new vertex arrays -----------------------------------
        const oldVertexPositions = this.vertexPositions!;
        const oldVertexNormals = this.vertexNormals as Float64Array | null;

        this.initVertexPositionsArray(j);
        if (oldVertexNormals !== null) {
            this.initVertexNormalsArray();
        }

        j = 0;
        for (i = 0; i < n; i++) {
            if (count[i]) {
                this.vertexPositions![3 * j + 0] = oldVertexPositions[3 * i + 0]!;
                this.vertexPositions![3 * j + 1] = oldVertexPositions[3 * i + 1]!;
                this.vertexPositions![3 * j + 2] = oldVertexPositions[3 * i + 2]!;
                if (oldVertexNormals !== null) {
                    this.vertexNormals![3 * j + 0] = oldVertexNormals[3 * i + 0]!;
                    this.vertexNormals![3 * j + 1] = oldVertexNormals[3 * i + 1]!;
                    this.vertexNormals![3 * j + 2] = oldVertexNormals[3 * i + 2]!;
                }
                j++;
            }
        }

        //- Calculate new triangle indices arrays -------------------------
        j = 0;
        const oldTriangleIndices = triangleIndexes;

        for (i = 0; i < Math.trunc(oldTriangleIndices.length / 3); i++) {
            a = oldTriangleIndices[3 * i + 0]!;
            b = oldTriangleIndices[3 * i + 1]!;
            c = oldTriangleIndices[3 * i + 2]!;
            if (a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n) {
                // This triangle is not copied
            } else {
                j++;
            }
        }

        this.initTriangleArrays(j);
        j = 0;
        for (i = 0; i < Math.trunc(oldTriangleIndices.length / 3); i++) {
            a = oldTriangleIndices[3 * i + 0]!;
            b = oldTriangleIndices[3 * i + 1]!;
            c = oldTriangleIndices[3 * i + 2]!;
            if (a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n) {
                // This triangle is not copied
            } else {
                this.triangleIndexes![3 * j + 0] = map[a]!;
                this.triangleIndexes![3 * j + 1] = map[b]!;
                this.triangleIndexes![3 * j + 2] = map[c]!;
                j++;
            }
        }
    }

    /**
    This method removes all currently selected vertices, and its
    corresponding incident triangles.

    Note that this method also removes "dangling" vertices, selected or not.
    */
    public removeSelectedVertices(): void {
        this.accelerationStructure = null;
        //-----------------------------------------------------------------
        if (this.vertexSelections === null) {
            return;
        }

        this.vertexNormals = null;
        this.vertexBinormals = null;
        this.vertexTangents = null;

        //-----------------------------------------------------------------
        let i: number;
        let a: number;
        let b: number;
        let c: number;
        const n = Math.trunc(TriangleMesh.nonNull(this.vertexPositions).length / 3);
        const triangleIndexes = TriangleMesh.nonNull(this.triangleIndexes);
        const selections = this.vertexSelections;
        for (i = 0; i < Math.trunc(triangleIndexes.length / 3); i++) {
            a = triangleIndexes[3 * i + 0]!;
            b = triangleIndexes[3 * i + 1]!;
            c = triangleIndexes[3 * i + 2]!;
            if (
                a < 0 ||
                a >= n ||
                b < 0 ||
                b >= n ||
                c < 0 ||
                c >= n ||
                JavaArray.get(selections, a) ||
                JavaArray.get(selections, b) ||
                JavaArray.get(selections, c)
            ) {
                triangleIndexes[3 * i + 0] = -1;
                triangleIndexes[3 * i + 1] = -1;
                triangleIndexes[3 * i + 2] = -1;
            }
        }

        //-----------------------------------------------------------------
        this.compact();

        //-----------------------------------------------------------------
        this.vertexSelections = null;

        this.calculateNormals();
    }

    private appendVertices(ev: ArrayListOfDoubles): void {
        let i: number;
        const vertexPositions = TriangleMesh.nonNull(this.vertexPositions);
        const newVertexPositions = new Float64Array(vertexPositions.length + ev.size());

        for (i = 0; i < vertexPositions.length; i++) {
            newVertexPositions[i] = vertexPositions[i]!;
        }
        for (i = 0; i < ev.size(); i++) {
            newVertexPositions[i + vertexPositions.length] = ev.get(i);
        }

        this.vertexPositions = newVertexPositions;
    }

    private appendTriangles(et: ArrayListOfInts): void {
        let i: number;
        const triangleIndexes = TriangleMesh.nonNull(this.triangleIndexes);
        const newTriangleIndices = new Int32Array(triangleIndexes.length + et.size());

        for (i = 0; i < triangleIndexes.length; i++) {
            newTriangleIndices[i] = triangleIndexes[i]!;
        }

        for (i = 0; i < et.size(); i++) {
            newTriangleIndices[i + triangleIndexes.length] = et.get(i);
        }

        this.triangleIndexes = newTriangleIndices;
    }

    private simpleTriangleCut(
        p: InfinitePlane,
        extraVertices: ArrayListOfDoubles,
        extraTriangles: ArrayListOfInts,
        nv: number,
        i: number,
        p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        let a: Vector3Dd;
        let b: Vector3Dd;
        let ma: Vector3Dd | null = null;
        let mb: Vector3Dd | null = null;
        const gia = new RayHit();
        const gib = new RayHit();
        a = p2.subtract(p1);
        b = p3.subtract(p1);
        a = a.normalized();
        b = b.normalized();

        const ra = new Ray(p1, a);
        const rb = new Ray(p1, b);

        const hitA = p.doIntersectionWithNegative(ra);
        if (hitA !== null) {
            if (p.doIntersectionFirstHit(hitA, gia)) {
                ma = gia.point;
            }
        }
        const hitB = p.doIntersectionWithNegative(rb);
        if (hitB !== null) {
            if (p.doIntersectionFirstHit(hitB, gib)) {
                mb = gib.point;
            }
        }

        extraTriangles.add(i);

        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);

        if (ma === null || mb === null) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut", "extraVertices is null!");
            return;
        }

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);

        extraVertices.add(mb.x());
        extraVertices.add(mb.y());
        extraVertices.add(mb.z());

        JavaArray.set(this.triangleIndexes, 3 * i + 0, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 1, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 2, -1);
    }

    private halfTriangleCut(
        p: InfinitePlane,
        extraVertices: ArrayListOfDoubles,
        extraTriangles: ArrayListOfInts,
        nv: number,
        i: number,
        j: number,
        _p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        let a: Vector3Dd;
        let ma: Vector3Dd | null = null;
        const gia = new RayHit();
        new RayHit(); // Java's unused `gib`
        a = p2.subtract(p3);
        a = a.normalized();

        const ra = new Ray(p2, a);

        const hitA = p.doIntersectionWithNegative(ra);
        if (hitA !== null) {
            if (p.doIntersectionFirstHit(hitA, gia)) {
                ma = gia.point;
            }
        }

        extraTriangles.add(i);

        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);

        if (ma === null) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut", "extraVertices is null!");
            return;
        }

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(j);

        JavaArray.set(this.triangleIndexes, 3 * i + 0, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 1, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 2, -1);
    }

    private doubleTriangleCut(
        p: InfinitePlane,
        extraVertices: ArrayListOfDoubles,
        extraTriangles: ArrayListOfInts,
        nv: number,
        i: number,
        j: number,
        p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        let a: Vector3Dd;
        let b: Vector3Dd;
        let ma: Vector3Dd | null = null;
        let mb: Vector3Dd | null = null;
        const gia = new RayHit();
        const gib = new RayHit();
        a = p1.subtract(p3);
        b = p2.subtract(p3);
        a = a.normalized();
        b = b.normalized();

        const ra = new Ray(p3, a);
        const rb = new Ray(p3, b);

        const hitA = p.doIntersectionWithNegative(ra);
        if (hitA !== null) {
            if (p.doIntersectionFirstHit(hitA, gia)) {
                ma = gia.point;
            }
        }
        const hitB = p.doIntersectionWithNegative(rb);
        if (hitB !== null) {
            if (p.doIntersectionFirstHit(hitB, gib)) {
                mb = gib.point;
            }
        }

        //-----------------------------------------------------------------
        extraTriangles.add(3 * JavaArray.get(this.triangleIndexes, 3 * i + 0) + 0);

        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);

        if (ma === null || mb === null) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut", "extraVertices is null!");
            return;
        }

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);

        extraTriangles.add(i); // for next tri.
        extraTriangles.add(Math.trunc(extraVertices.size() / 3) + nv);
        extraTriangles.add(j);

        extraVertices.add(mb.x());
        extraVertices.add(mb.y());
        extraVertices.add(mb.z());

        //-----------------------------------------------------------------
        JavaArray.set(this.triangleIndexes, 3 * i + 0, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 1, -1);
        JavaArray.set(this.triangleIndexes, 3 * i + 2, -1);
    }

    /**
    Given current mesh and a plane p, this method modifies the current mesh,
    leaving on it only the triangles which lies on the INSIDE part of the
    plane `p`. For triangles cutting the plane, new triangles are generated.
    */
    public slice(p: InfinitePlane): void {
        let i: number;
        let p1: Vector3Dd;
        let p2: Vector3Dd;
        let p3: Vector3Dd;
        let t1: number;
        let t2: number;
        let t3: number;
        const OUTSIDE = TriangleMesh.OUTSIDE;
        const LIMIT = TriangleMesh.LIMIT;
        const INSIDE = TriangleMesh.INSIDE;

        const extraTriangles = new ArrayListOfInts(10);
        const extraVertices = new ArrayListOfDoubles(10);

        const nv = Math.trunc(TriangleMesh.nonNull(this.vertexPositions).length / 3);

        for (i = 0; i < Math.trunc(TriangleMesh.nonNull(this.triangleIndexes).length / 3); i++) {
            p1 = this.trianglePosition(i, 0);

            p2 = this.trianglePosition(i, 1);

            p3 = this.trianglePosition(i, 2);

            t1 = p.doContainmentTestHalfSpace(p1, VSDK.EPSILON);
            t2 = p.doContainmentTestHalfSpace(p2, VSDK.EPSILON);
            t3 = p.doContainmentTestHalfSpace(p3, VSDK.EPSILON);

            const ti = this.triangleIndexes!;
            if (
                (t1 === OUTSIDE && t2 === OUTSIDE && t3 === OUTSIDE) ||
                (t1 === LIMIT && t2 === OUTSIDE && t3 === OUTSIDE) ||
                (t1 === OUTSIDE && t2 === LIMIT && t3 === OUTSIDE) ||
                (t1 === OUTSIDE && t2 === OUTSIDE && t3 === LIMIT) ||
                (t1 === LIMIT && t2 === LIMIT && t3 === OUTSIDE) ||
                (t1 === LIMIT && t2 === OUTSIDE && t3 === LIMIT) ||
                (t1 === OUTSIDE && t2 === LIMIT && t3 === LIMIT)
            ) {
                ti[3 * i + 0] = -1;
                ti[3 * i + 1] = -1;
                ti[3 * i + 2] = -1;
            } else if (t1 === LIMIT && t2 === LIMIT && t3 === LIMIT) {
                const a = p2.subtract(p1);
                const b = p3.subtract(p1);
                const n = a.crossProduct(b);
                if (n.dotProduct(p.getNormal()) > 0) {
                    ti[3 * i + 0] = -1;
                    ti[3 * i + 1] = -1;
                    ti[3 * i + 2] = -1;
                }
            } else if (
                (t1 === LIMIT && t2 === INSIDE && t3 === INSIDE) ||
                (t1 === INSIDE && t2 === LIMIT && t3 === INSIDE) ||
                (t1 === INSIDE && t2 === INSIDE && t3 === LIMIT) ||
                (t1 === LIMIT && t2 === LIMIT && t3 === INSIDE) ||
                (t1 === LIMIT && t2 === INSIDE && t3 === LIMIT) ||
                (t1 === INSIDE && t2 === LIMIT && t3 === LIMIT) ||
                (t1 === INSIDE && t2 === INSIDE && t3 === INSIDE)
            ) {
                // Untested?
            } else {
                const k = 3 * ti[3 * i + 0]!;
                if (t1 === INSIDE && t2 === OUTSIDE && t3 === OUTSIDE) {
                    this.simpleTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, p1, p2, p3);
                } else if (t2 === INSIDE && t1 === OUTSIDE && t3 === OUTSIDE) {
                    this.simpleTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, p2, p1, p3);
                } else if (t3 === INSIDE && t1 === OUTSIDE && t2 === OUTSIDE) {
                    this.simpleTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, p3, p1, p2);
                } else if (t1 === INSIDE && t2 === INSIDE && t3 === OUTSIDE) {
                    this.doubleTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 1, p1, p2, p3);
                } else if (t1 === INSIDE && t3 === INSIDE && t2 === OUTSIDE) {
                    this.doubleTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 2, p1, p3, p2);
                } else if (t2 === INSIDE && t3 === INSIDE && t1 === OUTSIDE) {
                    this.doubleTriangleCut(p, extraVertices, extraTriangles, nv, k + 1, k + 2, p2, p3, p1);
                } else if (t1 === INSIDE && t2 === LIMIT && t3 === OUTSIDE) {
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 1, p2, p1, p3);
                } else if (t2 === INSIDE && t1 === LIMIT && t3 === OUTSIDE) {
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 0, p1, p2, p3);
                } else if (t2 === INSIDE && t3 === LIMIT && t1 === OUTSIDE) {
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 2, p3, p2, p1);
                } else if (t3 === INSIDE && t2 === LIMIT && t1 === OUTSIDE) {
                    console.log("MyCase4");
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 1, p2, p3, p1);
                } else if (t3 === INSIDE && t1 === LIMIT && t2 === OUTSIDE) {
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 0, p1, p3, p2);
                } else if (t1 === INSIDE && t3 === LIMIT && t2 === OUTSIDE) {
                    this.halfTriangleCut(p, extraVertices, extraTriangles, nv, k + 0, k + 2, p3, p1, p2);
                } else {
                    console.log("Unhandled case: ");
                    console.log("  - P1: " + t1);
                    console.log("  - P2: " + t2);
                    console.log("  - P3: " + t3);
                }
            }
        }

        //-----------------------------------------------------------------
        this.appendVertices(extraVertices);
        this.appendTriangles(extraTriangles);

        //-----------------------------------------------------------------
        this.compact();

        //-----------------------------------------------------------------
        this.calculateNormals();
    }

    private static nonNull<T>(value: T | null): T {
        if (value === null) {
            throw new NullPointerException();
        }
        return value;
    }
}
