import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Containment } from "../../../processing/Containment.js";
import type { Image } from "../../../media/Image.js";
import type { SimpleMaterial } from "../../material/SimpleMaterial.js";
import { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { Triangle } from "../element/Triangle.js";
import { Vertex } from "../element/Vertex.js";
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
import { VSDK } from "../../../common/VSDK.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { TriangleMeshGroup } from "./TriangleMeshGroup.js";
import { TriangleAccelerationStructure } from "../boundingVolumeHierarchy/TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "../boundingVolumeHierarchy/TriangleMeshAccelerationMethod.js";
// Installs `TriangleAccelerationStructure.create` for WALD2007_BINNED_SAH.
import "../boundingVolumeHierarchy/BinnedSahTriangleHierarchy.js";
import "../boundingVolumeHierarchy/GridTriangleHierarchy.js";

/** Indexed triangle mesh with optional vertex attributes and material spans. */
export class TriangleMesh extends Surface<Ray, RayHit> {
    private name = "default";
    /**
    Whether raster renderers must draw both windings of this surface.
    MIT .ray scenes use two-sided triangle intersection, so their imported
    meshes set this flag to keep the GPU result consistent with raytracing.
    */
    private doubleSided = false;
    private vertexPositions: Float64Array | null = null;
    private vertexNormals: Float64Array | null = null;
    private vertexBinormals: Float64Array | null = null;
    private vertexTangents: Float64Array | null = null;
    private vertexColors: Float64Array | null = null;
    private vertexSelections: boolean[] | null = null;
    private vertexUvs: Float64Array | null = null;
    private triangleIndexes: Int32Array | null = null;
    private triangleNormals: Float64Array | null = null;
    private incidentTrianglesPerVertexArray: number[][] | null = null;
    private materials: SimpleMaterial[] | null = null;
    private textures: Image[] | null = null;
    private textureRanges: number[][] | null = null;
    private materialRanges: number[][] | null = null;
    private readonly intersectionTriangleIndex = new TriangleMeshIntersectionIndex();
    // Optional ray acceleration structure, see prepareAccelerationStructure
    private accelerationStructure: TriangleAccelerationStructure | null = null;
    public getName(): string {
        return this.name;
    }
    public setName(name: string): void {
        this.name = name;
    }
    public isDoubleSided(): boolean {
        return this.doubleSided;
    }
    public setDoubleSided(doubleSided: boolean): void {
        this.doubleSided = doubleSided;
    }
    public override clone(other?: TriangleMesh): TriangleMesh | void {
        if (other !== undefined) {
            this.copyFrom(other);
            return;
        }
        const copy = new TriangleMesh();
        copy.copyFrom(this);
        return copy;
    }
    private copyFrom(other: TriangleMesh): void {
        this.name = other.name;
        this.doubleSided = other.doubleSided;
        this.vertexPositions = other.vertexPositions?.slice() ?? null;
        this.vertexNormals = other.vertexNormals?.slice() ?? null;
        this.vertexBinormals = other.vertexBinormals?.slice() ?? null;
        this.vertexTangents = other.vertexTangents?.slice() ?? null;
        this.vertexColors = other.vertexColors?.slice() ?? null;
        this.vertexSelections = other.vertexSelections?.slice() ?? null;
        this.vertexUvs = other.vertexUvs?.slice() ?? null;
        this.triangleIndexes = other.triangleIndexes?.slice() ?? null;
        this.triangleNormals = other.triangleNormals?.slice() ?? null;
        this.materials = other.materials?.slice() ?? null;
        this.textures = other.textures?.slice() ?? null;
        this.textureRanges = other.textureRanges?.map((x) => x.slice()) ?? null;
        this.materialRanges = other.materialRanges?.map((x) => x.slice()) ?? null;
        this.accelerationStructure = other.accelerationStructure !== null ? other.accelerationStructure.copy() : null;
        this.calculateNormals();
    }
    public initVertexPositionsArray(n: number): void {
        this.accelerationStructure = null;
        this.vertexPositions = new Float64Array(n * 3);
    }
    public initVertexNormalsArray(): void {
        this.vertexNormals = new Float64Array(this.getNumVertices() * 3);
    }
    public initVertexBinormalsArray(): void {
        this.vertexBinormals = new Float64Array(this.getNumVertices() * 3);
    }
    public initVertexTangentsArray(): void {
        this.vertexTangents = new Float64Array(this.getNumVertices() * 3);
    }
    public initVertexColorsArray(): void {
        this.vertexColors = new Float64Array(this.getNumVertices() * 3);
    }
    public initVertexUvsArray(): void {
        this.vertexUvs = new Float64Array(this.getNumVertices() * 2);
    }
    public initIncidentTrianglesPerVertexArray(): void {
        this.incidentTrianglesPerVertexArray = Array.from({ length: this.getNumVertices() }, () => []);
    }
    public initTriangleArrays(n: number): void {
        this.accelerationStructure = null;
        this.triangleIndexes = new Int32Array(n * 3);
        this.triangleNormals = new Float64Array(n * 3);
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
    public setVertexes(
        vertices: Vertex[],
        withNormals = true,
        withBinormals = false,
        withTangents = false,
        withUvs = false,
    ): void {
        this.initVertexPositionsArray(vertices.length);
        if (withNormals) this.initVertexNormalsArray();
        if (withBinormals) this.initVertexBinormalsArray();
        if (withTangents) this.initVertexTangentsArray();
        if (withUvs) this.initVertexUvsArray();
        for (let i = 0; i < vertices.length; i++) this.writeVertex(i, vertices[i]!);
    }
    public setTriangles(triangles: Triangle[]): void {
        this.initTriangleArrays(triangles.length);
        for (let i = 0; i < triangles.length; i++) this.setTriangleAt(i, triangles[i]!);
    }
    public getVertexAt(i: number, vertex: Vertex): void {
        this.assertVertex(i);
        vertex.setPosition(this.position(i));
        if (this.vertexNormals !== null) vertex.setNormal(this.vector(this.vertexNormals, i));
        if (this.vertexBinormals !== null) vertex.setBinormal(this.vector(this.vertexBinormals, i));
        if (this.vertexTangents !== null) vertex.setTangent(this.vector(this.vertexTangents, i));
        if (this.vertexUvs !== null) {
            vertex.setU(this.vertexUvs[i * 2]!);
            vertex.setV(this.vertexUvs[i * 2 + 1]!);
        }
    }
    public setVertexAt(i: number, vertex: Vertex): void {
        this.accelerationStructure = null;
        this.assertVertex(i);
        this.writeVertex(i, vertex);
    }
    public setTriangleAt(i: number, triangle: Triangle): void {
        this.accelerationStructure = null;
        if (this.triangleIndexes === null || i < 0 || i >= this.getNumTriangles())
            throw new RangeError("Triangle index out of bounds");
        const indices = [triangle.getPoint0(), triangle.getPoint1(), triangle.getPoint2()];
        for (const index of indices) this.assertVertex(index);
        this.triangleIndexes.set(indices, i * 3);
        if (this.triangleNormals !== null) this.setVector(this.triangleNormals, i, triangle.getNormal());
    }
    public getNumVertices(): number {
        return this.vertexPositions?.length === undefined ? 0 : this.vertexPositions.length / 3;
    }
    public getNumTriangles(): number {
        return this.triangleIndexes?.length === undefined ? 0 : this.triangleIndexes.length / 3;
    }
    public getVertexSelections(): boolean[] {
        return (this.vertexSelections ??= Array.from({ length: this.getNumVertices() }, () => false));
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
    public getMaterials(): SimpleMaterial[] | null {
        return this.materials;
    }
    public setMaterials(x: SimpleMaterial[]): void {
        this.materials = x;
    }
    public getTextures(): Image[] | null {
        return this.textures;
    }
    public setTextures(x: Image[]): void {
        this.textures = x;
    }
    public getTextureAt(i: number): Image | undefined {
        return this.textures?.[i];
    }
    public setTextureAt(i: number, x: Image): void {
        if (this.textures === null) throw new Error("Textures are not initialized");
        this.textures[i] = x;
    }
    public getTextureRanges(): number[][] | null {
        return this.textureRanges;
    }
    public getTextureRangeAt(i: number): number[] | undefined {
        return this.textureRanges?.[i];
    }
    public setTextureRanges(x: number[][]): void {
        this.textureRanges = x;
    }
    public getMaterialRanges(): number[][] | null {
        return this.materialRanges;
    }
    public getMaterialRangeAt(i: number): number[] | undefined {
        return this.materialRanges?.[i];
    }
    public setMaterialRanges(x: number[][]): void {
        this.materialRanges = x;
    }
    public calculateNormals(): void {
        if (this.vertexPositions === null || this.triangleIndexes === null) return;
        this.triangleNormals = new Float64Array(this.getNumTriangles() * 3);
        const sums = Array.from({ length: this.getNumVertices() }, () => new Vector3Dd());
        this.initIncidentTrianglesPerVertexArray();
        for (let i = 0; i < this.getNumTriangles(); i++) {
            const [a, b, c] = this.triangle(i);
            const normal = this.position(b)
                .subtract(this.position(a))
                .crossProduct(this.position(c).subtract(this.position(a)))
                .normalized();
            this.setVector(this.triangleNormals, i, normal);
            for (const index of [a, b, c]) {
                sums[index] = sums[index]!.add(normal);
                this.incidentTrianglesPerVertexArray![index]!.push(i);
            }
        }
        this.initVertexNormalsArray();
        for (let i = 0; i < sums.length; i++) this.setVector(this.vertexNormals!, i, sums[i]!.normalized());
    }
    public reorientateNormals(): void {
        if (this.vertexNormals === null) return;
        for (let i = 0; i < this.getNumTriangles(); i++) {
            const [a, b, c] = this.triangle(i);
            const normal = this.position(b)
                .subtract(this.position(a))
                .crossProduct(this.position(c).subtract(this.position(a)))
                .normalized();
            if (this.triangleNormals !== null) this.setVector(this.triangleNormals, i, normal);
            for (const index of [a, b, c]) {
                const n = this.vector(this.vertexNormals, index);
                if (normal.dotProduct(n) < 0) this.setVector(this.vertexNormals, index, n.multiply(-1));
            }
        }
    }
    // Mathematical copy; scene-owned materials and textures are not copied.
    public override copyGeometry(): Geometry | null {
        const copy = this.clone() as TriangleMesh;
        copy.materials = [];
        copy.textures = [];
        copy.materialRanges = [];
        copy.textureRanges = [];
        return copy;
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

    /** C++ returns a read-only view of the stored array; this port returns the array itself. */
    public getVertexPositionsView(): Float64Array | null {
        return this.vertexPositions;
    }
    public getTriangleIndexesView(): Int32Array | null {
        return this.triangleIndexes;
    }
    public getTriangleNormalsView(): Float64Array | null {
        return this.triangleNormals;
    }

    public doIntersectionInformation(): number {
        return this.intersectionTriangleIndex.get();
    }
    /**
    Makes `doIntersectionFirstHit` use the given method: builds its
    acceleration structure (single threaded) unless one of the same method
    is already built for the current sizes; `BRUTE_FORCE` releases it. Also
    computes missing triangle normals. Call it before tracing from several
    threads: afterwards, calls with the same method only read. The result of
    the intersection does not depend on the method.

    The structure is a snapshot: the mutators of this class release it, but
    writes through the array getters (`getVertexPositions`,
    `getTriangleIndexes`) do not, so call `releaseAccelerationStructure` or
    this method again after them.
    */
    public prepareAccelerationStructure(method: TriangleMeshAccelerationMethod): void {
        // Done here, single threaded, instead of lazily from the tracing threads.
        if (this.triangleNormals === null || this.triangleNormals.length < this.getNumTriangles() * 3)
            this.calculateNormals();
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
            structure.build(this.vertexPositions ?? new Float64Array(0), this.triangleIndexes ?? new Int32Array(0));
        this.accelerationStructure = structure;
    }
    public releaseAccelerationStructure(): void {
        this.accelerationStructure = null;
    }
    /** Current structure, null when the brute force loop is used */
    public getAccelerationStructure(): TriangleAccelerationStructure | null {
        return this.accelerationStructure;
    }
    /** When a structure built for the current sizes exists: writes what
    `getMinMax` would return, without walking the vertices, and returns
    true */
    public getPreparedMinMax(outMinMax: number[] | Float64Array): boolean {
        const structure = this.accelerationStructure;
        if (structure === null || !structure.matches(this.getNumVertices(), this.getNumTriangles())) return false;
        structure.getVertexBounds(outMinMax);
        return true;
    }
    public doIntersectionFirstHit(ray: Ray, out: RayHit, triangleInformation?: Int32Array | number[]): boolean {
        return this.doIntersectionInternal(ray, out, triangleInformation);
    }
    // cpp `doIntersectionInternal`: nearest Moller-Trumbore hit, flat normal
    // first, then `interpolateTriangleData` with the hit's barycentrics.
    private doIntersectionInternal(ray: Ray, out: RayHit | null, triangleInformation?: Int32Array | number[]): boolean {
        if (this.getNumTriangles() === 0) {
            this.intersectionTriangleIndex.set(-1);
            if (triangleInformation !== undefined) triangleInformation[0] = -1;
            return false;
        }
        if (this.triangleNormals === null || this.triangleNormals.length < this.getNumTriangles() * 3)
            this.calculateNormals();

        const bestT = [1e308];
        const bestTri = [-1];
        const bestU = [0],
            bestV = [0];
        let found: boolean;

        const structure = this.accelerationStructure;
        if (structure !== null && structure.matches(this.getNumVertices(), this.getNumTriangles())) {
            found = structure.firstHit(
                ray,
                this.vertexPositions ?? new Float64Array(0),
                this.triangleIndexes ?? new Int32Array(0),
                bestT,
                bestU,
                bestV,
                bestTri,
            );
        } else {
            found = TriangleAccelerationStructure.firstHitBruteForce(
                ray,
                this.vertexPositions ?? new Float64Array(0),
                this.triangleIndexes ?? new Int32Array(0),
                bestT,
                bestU,
                bestV,
                bestTri,
            );
        }

        if (!found) {
            this.intersectionTriangleIndex.set(-1);
            if (triangleInformation !== undefined) triangleInformation[0] = -1;
            return false;
        }
        this.intersectionTriangleIndex.set(bestTri[0]!);
        if (triangleInformation !== undefined) triangleInformation[0] = bestTri[0]!;

        if (out !== null) {
            out.setRay(ray.withT(bestT[0]!));
            out.point = ray.getOrigin().add(ray.getDirection().multiply(bestT[0]!));
            out.normal = this.vector(this.triangleNormals!, bestTri[0]!).normalized();
            out.tangent = new Vector3Dd();
            out.u = 0;
            out.v = 0;
            this.interpolateTriangleData(bestTri[0]!, bestU[0]!, bestV[0]!, out, ray.getDirection());
            this.fillMaterialAndTexture(bestTri[0]!, out);
        }
        return true;
    }
    public override doExtraInformation(ray: Ray, t: number, out: RayHit): void {
        this.doIntersectionFirstHit(ray.withT(t), out);
    }
    public override doContainmentTest(point: Vector3Dd, tolerance: number): number {
        for (let i = 0; i < this.getNumTriangles(); i++) {
            const [a, b, c] = this.triangle(i);
            if (
                Triangle.containmentTest(this.position(a), this.position(b), this.position(c), point, tolerance) !==
                Containment.OUTSIDE
            )
                return Containment.LIMIT;
        }
        return Containment.OUTSIDE;
    }
    public getMinMax(): Float64Array {
        const result = new Float64Array([Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity]);
        for (let i = 0; i < this.getNumVertices(); i++) {
            const p = this.position(i);
            result[0] = Math.min(result[0]!, p.x());
            result[1] = Math.min(result[1]!, p.y());
            result[2] = Math.min(result[2]!, p.z());
            result[3] = Math.max(result[3]!, p.x());
            result[4] = Math.max(result[4]!, p.y());
            result[5] = Math.max(result[5]!, p.z());
        }
        return result;
    }
    public exportToTriangleMeshGroup(): TriangleMeshGroup {
        return new TriangleMeshGroup([this]);
    }
    public compact(): void {
        this.accelerationStructure = null;
        if (this.vertexPositions === null || this.triangleIndexes === null) return;
        const used = new Set<number>();
        const valid: number[][] = [];
        for (let i = 0; i < this.getNumTriangles(); i++) {
            const triangle = this.triangle(i);
            if (triangle.every((index) => index >= 0 && index < this.getNumVertices())) {
                valid.push(triangle);
                triangle.forEach((index) => used.add(index));
            }
        }
        const map = new Map<number, number>();
        const vertices = [...used]
            .sort((a, b) => a - b)
            .map((old, next) => {
                map.set(old, next);
                return this.position(old);
            });
        this.setVertexes(
            vertices.map((x) => new Vertex(x)),
            false,
        );
        this.initTriangleArrays(valid.length);
        valid.forEach((triangle, i) =>
            this.triangleIndexes!.set(
                triangle.map((x) => map.get(x)!),
                i * 3,
            ),
        );
        this.calculateNormals();
    }

    /** Retains the Java clipping algorithm and its original case ordering. */
    public slice(plane: import("./InfinitePlane.js").InfinitePlane): void {
        this.accelerationStructure = null;
        if (this.vertexPositions === null || this.triangleIndexes === null) return;
        const extraVertices: number[] = [],
            extraTriangles: number[] = [],
            nv = this.getNumVertices();
        for (let i = 0; i < this.triangleIndexes.length / 3; i++) {
            const i0 = this.triangleIndexes[3 * i]!,
                i1 = this.triangleIndexes[3 * i + 1]!,
                i2 = this.triangleIndexes[3 * i + 2]!;
            const p1 = new Vector3Dd(
                this.vertexPositions[3 * i0]!,
                this.vertexPositions[3 * i0 + 1]!,
                this.vertexPositions[3 * i0 + 2]!,
            );
            const p2 = new Vector3Dd(
                this.vertexPositions[3 * i1]!,
                this.vertexPositions[3 * i1 + 1]!,
                this.vertexPositions[3 * i1 + 2]!,
            );
            const p3 = new Vector3Dd(
                this.vertexPositions[3 * i2]!,
                this.vertexPositions[3 * i2 + 1]!,
                this.vertexPositions[3 * i2 + 2]!,
            );
            const t1 = plane.doContainmentTestHalfSpace(p1, 1e-6),
                t2 = plane.doContainmentTestHalfSpace(p2, 1e-6),
                t3 = plane.doContainmentTestHalfSpace(p3, 1e-6);
            const outside = Containment.OUTSIDE,
                limit = Containment.LIMIT,
                inside = Containment.INSIDE;
            if (
                (t1 === outside && t2 === outside && t3 === outside) ||
                (t1 === limit && t2 === outside && t3 === outside) ||
                (t1 === outside && t2 === limit && t3 === outside) ||
                (t1 === outside && t2 === outside && t3 === limit) ||
                (t1 === limit && t2 === limit && t3 === outside) ||
                (t1 === limit && t2 === outside && t3 === limit) ||
                (t1 === outside && t2 === limit && t3 === limit)
            )
                this.triangleIndexes.set([-1, -1, -1], 3 * i);
            else if (t1 === limit && t2 === limit && t3 === limit) {
                if (p2.subtract(p1).crossProduct(p3.subtract(p1)).dotProduct(plane.getNormal()) > 0)
                    this.triangleIndexes.set([-1, -1, -1], 3 * i);
            } else if (
                (t1 === limit && t2 === inside && t3 === inside) ||
                (t1 === inside && t2 === limit && t3 === inside) ||
                (t1 === inside && t2 === inside && t3 === limit) ||
                (t1 === limit && t2 === limit && t3 === inside) ||
                (t1 === limit && t2 === inside && t3 === limit) ||
                (t1 === inside && t2 === limit && t3 === limit) ||
                (t1 === inside && t2 === inside && t3 === inside)
            ) {
                continue;
            } else if (t1 === inside && t2 === outside && t3 === outside)
                this.simpleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, p1, p2, p3);
            else if (t2 === inside && t1 === outside && t3 === outside)
                this.simpleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, p2, p1, p3);
            else if (t3 === inside && t1 === outside && t2 === outside)
                this.simpleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, p3, p1, p2);
            else if (t1 === inside && t2 === inside && t3 === outside)
                this.doubleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i1, p1, p2, p3);
            else if (t1 === inside && t3 === inside && t2 === outside)
                this.doubleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i2, p1, p3, p2);
            else if (t2 === inside && t3 === inside && t1 === outside)
                this.doubleTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i1, 3 * i2, p2, p3, p1);
            else if (t1 === inside && t2 === limit && t3 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i1, p2, p1, p3);
            else if (t2 === inside && t1 === limit && t3 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i0, p1, p2, p3);
            else if (t2 === inside && t3 === limit && t1 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i2, p3, p2, p1);
            else if (t3 === inside && t2 === limit && t1 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i1, p2, p3, p1);
            else if (t3 === inside && t1 === limit && t2 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i0, p1, p3, p2);
            else if (t1 === inside && t3 === limit && t2 === outside)
                this.halfTriangleCut(plane, extraVertices, extraTriangles, nv, 3 * i0, 3 * i2, p3, p1, p2);
        }
        this.appendSliceVertices(extraVertices);
        this.appendSliceTriangles(extraTriangles);
        this.compact();
        this.calculateNormals();
    }
    private appendSliceVertices(values: number[]): void {
        if (this.vertexPositions !== null) {
            const out = new Float64Array(this.vertexPositions.length + values.length);
            out.set(this.vertexPositions);
            out.set(values, this.vertexPositions.length);
            this.vertexPositions = out;
        }
    }
    private appendSliceTriangles(values: number[]): void {
        if (this.triangleIndexes !== null) {
            const out = new Int32Array(this.triangleIndexes.length + values.length);
            out.set(this.triangleIndexes);
            out.set(values, this.triangleIndexes.length);
            this.triangleIndexes = out;
        }
    }
    private intersectionVertex(
        plane: import("./InfinitePlane.js").InfinitePlane,
        origin: Vector3Dd,
        toward: Vector3Dd,
    ): Vector3Dd | null {
        const direction = toward.subtract(origin).normalized(),
            hit = plane.doIntersectionWithNegative(new Ray(origin, direction));
        if (hit === null) return null;
        const out = new RayHit();
        return plane.doIntersectionFirstHit(hit, out) ? out.point : null;
    }
    private simpleTriangleCut(
        plane: import("./InfinitePlane.js").InfinitePlane,
        ev: number[],
        et: number[],
        nv: number,
        index: number,
        p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        const a = this.intersectionVertex(plane, p1, p2),
            b = this.intersectionVertex(plane, p1, p3);
        et.push(index, ev.length / 3 + nv);
        if (a === null || b === null) return;
        ev.push(a.x(), a.y(), a.z());
        et.push(ev.length / 3 + nv);
        ev.push(b.x(), b.y(), b.z());
        this.triangleIndexes!.set([-1, -1, -1], 3 * index);
    }
    private halfTriangleCut(
        plane: import("./InfinitePlane.js").InfinitePlane,
        ev: number[],
        et: number[],
        nv: number,
        index: number,
        second: number,
        p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        const a = this.intersectionVertex(plane, p2, p3);
        et.push(index, ev.length / 3 + nv);
        if (a === null) return;
        ev.push(a.x(), a.y(), a.z());
        et.push(second);
        this.triangleIndexes!.set([-1, -1, -1], 3 * index);
    }
    private doubleTriangleCut(
        plane: import("./InfinitePlane.js").InfinitePlane,
        ev: number[],
        et: number[],
        nv: number,
        index: number,
        second: number,
        p1: Vector3Dd,
        p2: Vector3Dd,
        p3: Vector3Dd,
    ): void {
        const a = this.intersectionVertex(plane, p3, p1),
            b = this.intersectionVertex(plane, p3, p2);
        et.push(this.triangleIndexes![3 * index]!, ev.length / 3 + nv);
        if (a === null || b === null) return;
        ev.push(a.x(), a.y(), a.z());
        et.push(ev.length / 3 + nv);
        ev.push(b.x(), b.y(), b.z());
        et.push(index, ev.length / 3 + nv, second);
        this.triangleIndexes!.set([-1, -1, -1], 3 * index);
    }
    public removeSelectedVertices(): void {
        this.accelerationStructure = null;
        if (this.vertexSelections === null || this.triangleIndexes === null) return;
        for (let i = 0; i < this.getNumTriangles(); i++) {
            const indices = this.triangle(i);
            if (indices.some((x) => this.vertexSelections![x])) this.triangleIndexes.set([-1, -1, -1], i * 3);
        }
        this.vertexSelections = null;
        this.compact();
    }
    public override toString(): string {
        return `- TriangleMesh\n  - Number of triangles: ${this.getNumTriangles()}\n  - Number of vertexes: ${this.getNumVertices()}\n`;
    }
    private interpolateTriangleData(tri: number, u: number, v: number, out: RayHit, rayDirection: Vector3Dd): void {
        const [i0, i1, i2] = this.triangle(tri);
        const w = 1.0 - u - v;

        if (out.needsNormal()) {
            if (
                this.vertexNormals !== null &&
                this.vertexNormals.length >= this.getNumVertices() * 3 &&
                this.vertexUvs !== null &&
                this.vertexUvs.length >= this.getNumVertices() * 2
            ) {
                const vn = this.vertexNormals;
                const n = new Vector3Dd(
                    w * vn[i0 * 3]! + u * vn[i1 * 3]! + v * vn[i2 * 3]!,
                    w * vn[i0 * 3 + 1]! + u * vn[i1 * 3 + 1]! + v * vn[i2 * 3 + 1]!,
                    w * vn[i0 * 3 + 2]! + u * vn[i1 * 3 + 2]! + v * vn[i2 * 3 + 2]!,
                );
                out.normal = n.normalized();
            } else {
                out.normal = this.vector(this.triangleNormals!, tri).normalized();
            }
            if (out.normal.dotProduct(rayDirection) >= 0) out.normal = out.normal.multiply(-1);
        }

        if (
            out.needsTextureCoordinates() &&
            this.vertexUvs !== null &&
            this.vertexUvs.length >= this.getNumVertices() * 2
        ) {
            const uv = this.vertexUvs;
            out.u = w * uv[i0 * 2]! + u * uv[i1 * 2]! + v * uv[i2 * 2]!;
            out.v = w * uv[i0 * 2 + 1]! + u * uv[i1 * 2 + 1]! + v * uv[i2 * 2 + 1]!;
        }
    }
    private fillMaterialAndTexture(triangle: number, out: RayHit): void {
        out.material = this.materials?.[0] ?? null;
        for (let i = 0; this.materialRanges !== null && i < this.materialRanges.length - 1; i++)
            if (triangle >= this.materialRanges[i]![0]! && triangle < this.materialRanges[i + 1]![0]!) {
                out.material = this.materials?.[this.materialRanges[i + 1]![1]!] ?? null;
                break;
            }
        out.texture = null;
        for (let i = 0; this.textureRanges !== null && i < this.textureRanges.length - 1; i++)
            if (triangle >= this.textureRanges[i]![0]! && triangle < this.textureRanges[i + 1]![0]!) {
                out.texture = this.textures?.[this.textureRanges[i + 1]![1]! - 1] ?? null;
                break;
            }
    }
    private writeVertex(i: number, vertex: Vertex): void {
        this.setVector(this.vertexPositions!, i, vertex.position);
        if (this.vertexNormals !== null) this.setVector(this.vertexNormals, i, vertex.normal);
        if (this.vertexBinormals !== null) this.setVector(this.vertexBinormals, i, vertex.binormal);
        if (this.vertexTangents !== null) this.setVector(this.vertexTangents, i, vertex.tangent);
        if (this.vertexUvs !== null) {
            this.vertexUvs[i * 2] = vertex.u;
            this.vertexUvs[i * 2 + 1] = vertex.v;
        }
    }
    private assertVertex(i: number): void {
        if (!Number.isInteger(i) || i < 0 || i >= this.getNumVertices())
            throw new RangeError("Vertex index out of bounds");
    }
    private position(i: number): Vector3Dd {
        this.assertVertex(i);
        return this.vector(this.vertexPositions!, i);
    }
    private vector(a: Float64Array, i: number): Vector3Dd {
        return new Vector3Dd(a[i * 3]!, a[i * 3 + 1]!, a[i * 3 + 2]!);
    }
    private setVector(a: Float64Array, i: number, v: Vector3Dd): void {
        a[i * 3] = v.x();
        a[i * 3 + 1] = v.y();
        a[i * 3 + 2] = v.z();
    }
    private triangle(i: number): [number, number, number] {
        if (this.triangleIndexes === null || i < 0 || i >= this.getNumTriangles())
            throw new RangeError("Triangle index out of bounds");
        const n = i * 3;
        return [this.triangleIndexes[n]!, this.triangleIndexes[n + 1]!, this.triangleIndexes[n + 2]!];
    }
}
