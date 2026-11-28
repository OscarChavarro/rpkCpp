import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { Triangle } from "../element/Triangle.js";
import { Vertex } from "../element/Vertex.js";
import { Surface } from "./Surface.js";
import { TriangleMesh } from "./TriangleMesh.js";
import { TriangleMeshGroup } from "./TriangleMeshGroup.js";
import { Double } from "../../../../../java/lang/Double.js";
import { JavaArray } from "../../../../../java/lang/JavaArray.js";
import { NullPointerException } from "../../../../../java/lang/NullPointerException.js";

/**
This class represents a "basic" quad mesh. Its model is based in a set
of vertexes and triangles (the edges are not store explicitly, and there
can not be an edge not forming part of a quad).

As every model class or `Entity` in VSDK, this class only can represent
(store in memory) the mesh model. It doesn't provide persistence or rendering
functionality, as this could be found at `io` and `render` packages.
Nevertheles, this class will be highly coupled with both of those, so
making any change here will impact highly that code.

This class does not ensure nor impose data integrity, and this will be the
sole responsability of the cooperating utilities and applications.

Java's `double[]` / `int[]` arrays are `Float64Array` / `Int32Array` here;
element accesses go through `JavaArray`, so null arrays and out of range
indexes throw as in Java.
\todo  Document more this class (include samples and data structure diagrams)
*/
export class QuadMesh extends Surface<Ray, RayHit> {
    //= Class attributes ========================================================

    // Basic mesh data model
    private name: string = "default";

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
    // Consecutive pairs of u/v vertex texture coordinates (can be null)
    private vertexUvs: Float64Array | null;

    private incidentQuadsPerVertexArray: number[][] | null;

    /// Consecutive quads of p0/p1/p2/p3 indices into the vertex arrays
    private quadIndices: Int32Array | null;

    //= Basic class management methods ==========================================

    public constructor() {
        super();
        this.vertexPositions = null;
        this.vertexNormals = null;
        this.vertexBinormals = null;
        this.vertexTangents = null;
        this.vertexColors = null;
        this.vertexUvs = null;
        this.quadIndices = null;
        this.incidentQuadsPerVertexArray = null;
    }

    public getName(): string {
        return this.name;
    }

    public getVertexAt(i: number, vertex: Vertex): void {
        vertex.position = vertex.position.withX(JavaArray.get(this.vertexPositions, 3 * i));
        vertex.position = vertex.position.withY(JavaArray.get(this.vertexPositions, 3 * i + 1));
        vertex.position = vertex.position.withZ(JavaArray.get(this.vertexPositions, 3 * i + 2));
        vertex.normal = vertex.normal.withX(JavaArray.get(this.vertexNormals, 3 * i));
        vertex.normal = vertex.normal.withY(JavaArray.get(this.vertexNormals, 3 * i + 1));
        vertex.normal = vertex.normal.withZ(JavaArray.get(this.vertexNormals, 3 * i + 2));
        vertex.binormal = vertex.binormal.withX(JavaArray.get(this.vertexBinormals, 3 * i));
        vertex.binormal = vertex.binormal.withY(JavaArray.get(this.vertexBinormals, 3 * i + 1));
        vertex.binormal = vertex.binormal.withZ(JavaArray.get(this.vertexBinormals, 3 * i + 2));
        vertex.tangent = vertex.tangent.withX(JavaArray.get(this.vertexTangents, 3 * i));
        vertex.tangent = vertex.tangent.withY(JavaArray.get(this.vertexTangents, 3 * i + 1));
        vertex.tangent = vertex.tangent.withZ(JavaArray.get(this.vertexTangents, 3 * i + 2));
        vertex.u = JavaArray.get(this.vertexUvs, 2 * i);
        vertex.v = JavaArray.get(this.vertexUvs, 2 * i + 1);
    }

    public setName(name: string): void {
        this.name = name;
    }

    public initVertexPositionsArray(n: number): void {
        this.vertexPositions = new Float64Array(n * 3);
        this.incidentQuadsPerVertexArray = [];
    }

    public initVertexColorsArray(): void {
        const n = this.getNumVertices();
        this.vertexColors = new Float64Array(n * 3);
    }

    public initVertexNormalsArray(): void {
        const n = this.getNumVertices();
        this.vertexNormals = new Float64Array(n * 3);
    }

    /**
    This method provides a clear structured form of defining the mesh vertexes,
    but it is inefficient. Its use is discouraged for applications manipulating
    big meshes.
    */
    public setVertexes(vertexes: Vertex[]): void {
        let i: number;

        const n = vertexes.length;
        this.initVertexPositionsArray(n);
        this.initVertexNormalsArray();
        for (i = 0; i < n; i++) {
            const vertex = QuadMesh.nonNull(vertexes[i] ?? null);
            JavaArray.set(this.vertexPositions, 3 * i, vertex.position.x());
            JavaArray.set(this.vertexPositions, 3 * i + 1, vertex.position.y());
            JavaArray.set(this.vertexPositions, 3 * i + 2, vertex.position.z());
            JavaArray.set(this.vertexNormals, 3 * i, vertex.normal.x());
            JavaArray.set(this.vertexNormals, 3 * i + 1, vertex.normal.y());
            JavaArray.set(this.vertexNormals, 3 * i + 2, vertex.normal.z());
        }
    }

    public initQuadArrays(n: number): void {
        this.quadIndices = new Int32Array(n * 4);
    }

    /**
    Given a vertex structure and an `i` position, this method copies
    the information from the structure in to the i-th vertex arrays position.
    PRE: 0 <= i < vertexPositions.length/3
    */
    public setVertexAt(i: number, vertex: Vertex): void {
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
    public setQuadAt(i: number, p0: number, p1: number, p2: number, p3: number): void {
        JavaArray.set(this.quadIndices, 4 * i, p0);
        JavaArray.set(this.quadIndices, 4 * i + 1, p1);
        JavaArray.set(this.quadIndices, 4 * i + 2, p2);
        JavaArray.set(this.quadIndices, 4 * i + 3, p3);
    }

    public getNumVertices(): number {
        if (this.vertexPositions === null) return 0;
        return Math.trunc(this.vertexPositions.length / 3);
    }

    public getNumQuads(): number {
        if (this.quadIndices === null) return 0;
        return Math.trunc(this.quadIndices.length / 4);
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

    public getQuadIndices(): Int32Array | null {
        return this.quadIndices;
    }

    //= Fundamental geometry operations methods =================================

    public calculateNormals(): void {
        // TODO!
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

        for (i = 0; i < this.getNumVertices(); i++) {
            const x = this.vertexPositions![3 * i + 0]!;
            const y = this.vertexPositions![3 * i + 1]!;
            const z = this.vertexPositions![3 * i + 2]!;

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
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return a human readable representation of current QuadMesh information
    */
    public override toString(): string {
        let msg: string;
        const mm = this.getMinMax();
        let p: Vector3Dd;

        msg = "- QuadMesh ------------------------------------------------------------\n";
        msg += "  - Number of quads:" + this.getNumQuads() + "\n";
        msg += "  - Number of vertexes:" + this.getNumVertices() + "\n";
        p = new Vector3Dd(mm[0]!, mm[1]!, mm[2]!);
        msg += "  - MINMAX: " + p.toString();
        p = new Vector3Dd(mm[3]!, mm[4]!, mm[5]!);
        msg += " - " + p.toString() + "\n";
        msg += "---------------------------------------------------------------------------\n";
        return msg;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
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
        return this.exportToTriangleMeshGroup().doIntersectionFirstHit(inRay, outHit);
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if ((outData as RayHit | null) === null) {
            return;
        }
        this.exportToTriangleMeshGroup().doExtraInformation(inRay, inT, outData);
    }

    /**
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        return this.exportToTriangleMeshGroup().doContainmentTest(p, distanceTolerance);
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

    // Todo!
    public exportToTriangleMeshGroup(): TriangleMeshGroup {
        const group = new TriangleMeshGroup();
        const nVertices = this.getNumVertices();
        const nQuads = this.getNumQuads();
        if (nVertices <= 0 || nQuads <= 0) {
            return group;
        }

        const vertices: Vertex[] = new Array<Vertex>(nVertices);
        for (let i = 0; i < nVertices; i++) {
            const p = new Vector3Dd(
                this.vertexPositions![3 * i]!,
                this.vertexPositions![3 * i + 1]!,
                this.vertexPositions![3 * i + 2]!,
            );
            if (this.vertexNormals !== null) {
                const n = new Vector3Dd(
                    JavaArray.get(this.vertexNormals, 3 * i),
                    JavaArray.get(this.vertexNormals, 3 * i + 1),
                    JavaArray.get(this.vertexNormals, 3 * i + 2),
                );
                vertices[i] = new Vertex(p, n);
            } else {
                vertices[i] = new Vertex(p);
            }
        }

        const triangles: Triangle[] = new Array<Triangle>(nQuads * 2);
        for (let i = 0; i < nQuads; i++) {
            const p0 = this.quadIndices![4 * i]!;
            const p1 = this.quadIndices![4 * i + 1]!;
            const p2 = this.quadIndices![4 * i + 2]!;
            const p3 = this.quadIndices![4 * i + 3]!;
            triangles[2 * i] = new Triangle(p0, p1, p2);
            triangles[2 * i + 1] = new Triangle(p0, p2, p3);
        }

        const mesh = new TriangleMesh();
        mesh.setVertexes(vertices, this.vertexNormals !== null, false, false, false);
        mesh.setTriangles(triangles);
        mesh.calculateNormals();
        mesh.setName(this.name + "_triangulated");
        group.addMesh(mesh);
        return group;
    }

    private static nonNull<T>(value: T | null): T {
        if (value === null) {
            throw new NullPointerException();
        }
        return value;
    }
}
