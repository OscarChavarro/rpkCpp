import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Containment } from "../../../processing/Containment.js";
import { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { Triangle } from "../element/Triangle.js";
import { Vertex } from "../element/Vertex.js";
import { Surface } from "./Surface.js";
import { TriangleMesh } from "./TriangleMesh.js";
import { TriangleMeshGroup } from "./TriangleMeshGroup.js";

/** Indexed quad mesh. Each quad is tested as the two triangles (0,1,2) and (0,2,3). */
export class QuadMesh extends Surface<Ray, RayHit> {
    private name = "default";
    private vertexPositions: Float64Array | null = null;
    private vertexNormals: Float64Array | null = null;
    private vertexBinormals: Float64Array | null = null;
    private vertexTangents: Float64Array | null = null;
    private vertexColors: Float64Array | null = null;
    private vertexUvs: Float64Array | null = null;
    private quadIndices: Int32Array | null = null;
    private incidentQuadsPerVertexArray: number[][] | null = null;

    public getName(): string {
        return this.name;
    }
    public setName(name: string): void {
        this.name = name;
    }
    public initVertexPositionsArray(count: number): void {
        this.vertexPositions = new Float64Array(count * 3);
        this.incidentQuadsPerVertexArray = Array.from({ length: count }, () => []);
    }
    public initVertexColorsArray(): void {
        this.vertexColors = new Float64Array(this.getNumVertices() * 3);
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
    public initVertexUvsArray(): void {
        this.vertexUvs = new Float64Array(this.getNumVertices() * 2);
    }
    public initQuadArrays(count: number): void {
        this.quadIndices = new Int32Array(count * 4);
    }
    public getNumVertices(): number {
        return this.vertexPositions?.length === undefined ? 0 : this.vertexPositions.length / 3;
    }
    public getNumQuads(): number {
        return this.quadIndices?.length === undefined ? 0 : this.quadIndices.length / 4;
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

    public setVertexes(vertexes: Vertex[]): void {
        this.initVertexPositionsArray(vertexes.length);
        this.initVertexNormalsArray();
        for (let i = 0; i < vertexes.length; i++) this.setPositionAndNormal(i, vertexes[i]!);
    }
    public getVertexAt(index: number, vertex: Vertex): void {
        this.assertVertex(index);
        const p = this.position(index);
        vertex.setPosition(p);
        if (this.vertexNormals !== null) vertex.setNormal(this.vector(this.vertexNormals, index));
        if (this.vertexBinormals !== null) vertex.setBinormal(this.vector(this.vertexBinormals, index));
        if (this.vertexTangents !== null) vertex.setTangent(this.vector(this.vertexTangents, index));
        if (this.vertexUvs !== null) {
            vertex.setU(this.vertexUvs[index * 2]!);
            vertex.setV(this.vertexUvs[index * 2 + 1]!);
        }
    }
    public setVertexAt(index: number, vertex: Vertex): void {
        this.assertVertex(index);
        this.setPositionAndNormal(index, vertex);
        if (this.vertexBinormals !== null) this.setVector(this.vertexBinormals, index, vertex.binormal);
        if (this.vertexTangents !== null) this.setVector(this.vertexTangents, index, vertex.tangent);
        if (this.vertexUvs !== null) {
            this.vertexUvs[index * 2] = vertex.u;
            this.vertexUvs[index * 2 + 1] = vertex.v;
        }
    }
    public setQuadAt(index: number, p0: number, p1: number, p2: number, p3: number): void {
        if (this.quadIndices === null || index < 0 || index >= this.getNumQuads())
            throw new RangeError("Quad index out of bounds");
        for (const point of [p0, p1, p2, p3]) this.assertVertex(point);
        this.quadIndices.set([p0, p1, p2, p3], index * 4);
        for (const point of [p0, p1, p2, p3]) this.incidentQuadsPerVertexArray?.[point]?.push(index);
    }
    public calculateNormals(): void {
        if (this.vertexPositions === null) return;
        this.initVertexNormalsArray();
        const sums = Array.from({ length: this.getNumVertices() }, () => new Vector3Dd());
        for (let q = 0; q < this.getNumQuads(); q++) {
            const [a, b, c, d] = this.quad(q);
            const n1 = this.position(b)
                .subtract(this.position(a))
                .crossProduct(this.position(c).subtract(this.position(a)));
            const n2 = this.position(c)
                .subtract(this.position(a))
                .crossProduct(this.position(d).subtract(this.position(a)));
            for (const index of [a, b, c]) sums[index] = sums[index]!.add(n1);
            for (const index of [a, c, d]) sums[index] = sums[index]!.add(n2);
        }
        for (let i = 0; i < sums.length; i++) this.setVector(this.vertexNormals!, i, sums[i]!.normalized());
    }
    /** cpp: intersects the triangles of `exportToTriangleMeshGroup`. */
    public doIntersectionFirstHit(ray: Ray, out: RayHit): boolean {
        const g = this.exportToTriangleMeshGroup();
        return g.doIntersectionFirstHit(ray, out);
    }
    public override doExtraInformation(ray: Ray, t: number, out: RayHit): void {
        const g = this.exportToTriangleMeshGroup();
        g.doExtraInformation(ray, t, out);
    }
    public override doContainmentTest(point: Vector3Dd, tolerance: number): number {
        for (let q = 0; q < this.getNumQuads(); q++) {
            const [a, b, c, d] = this.quad(q);
            if (
                Triangle.containmentTest(this.position(a), this.position(b), this.position(c), point, tolerance) !==
                    Containment.OUTSIDE ||
                Triangle.containmentTest(this.position(a), this.position(c), this.position(d), point, tolerance) !==
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
    /**
    cpp `exportToTriangleMeshGroup`: a triangle mesh with copies of the vertex
    arrays and the triangles (0,1,2) and (0,2,3) of each quad, with its
    normals recalculated.
    */
    public exportToTriangleMeshGroup(): TriangleMeshGroup {
        const mesh = new TriangleMesh();
        mesh.setName(this.name);
        mesh.initVertexPositionsArray(this.getNumVertices());
        if (this.vertexPositions !== null) mesh.getVertexPositions()!.set(this.vertexPositions);
        if (this.vertexNormals !== null) {
            mesh.initVertexNormalsArray();
            mesh.getVertexNormals()!.set(this.vertexNormals);
        }
        if (this.vertexBinormals !== null) {
            mesh.initVertexBinormalsArray();
            mesh.getVertexBinormals()!.set(this.vertexBinormals);
        }
        if (this.vertexTangents !== null) {
            mesh.initVertexTangentsArray();
            mesh.getVertexTangents()!.set(this.vertexTangents);
        }
        if (this.vertexColors !== null) {
            mesh.initVertexColorsArray();
            mesh.getVertexColors()!.set(this.vertexColors);
        }
        if (this.vertexUvs !== null) {
            mesh.initVertexUvsArray();
            mesh.getVertexUvs()!.set(this.vertexUvs);
        }

        mesh.initTriangleArrays(this.getNumQuads() * 2);
        const tri = mesh.getTriangleIndexes()!;

        let t = 0;
        for (let i = 0; i < this.getNumQuads(); i++) {
            const p0 = this.quadIndices![4 * i]!;
            const p1 = this.quadIndices![4 * i + 1]!;
            const p2 = this.quadIndices![4 * i + 2]!;
            const p3 = this.quadIndices![4 * i + 3]!;
            tri[3 * t] = p0;
            tri[3 * t + 1] = p1;
            tri[3 * t + 2] = p2;
            t++;
            tri[3 * t] = p0;
            tri[3 * t + 1] = p2;
            tri[3 * t + 2] = p3;
            t++;
        }
        mesh.calculateNormals();

        const g = new TriangleMeshGroup();
        g.addMesh(mesh);
        return g;
    }
    public override toString(): string {
        return `- QuadMesh\n  - Number of quads: ${this.getNumQuads()}\n  - Number of vertexes: ${this.getNumVertices()}\n`;
    }
    private assertVertex(index: number): void {
        if (!Number.isInteger(index) || index < 0 || index >= this.getNumVertices())
            throw new RangeError("Vertex index out of bounds");
    }
    private position(index: number): Vector3Dd {
        this.assertVertex(index);
        return this.vector(this.vertexPositions!, index);
    }
    private vector(values: Float64Array, index: number): Vector3Dd {
        return new Vector3Dd(values[index * 3]!, values[index * 3 + 1]!, values[index * 3 + 2]!);
    }
    private setVector(values: Float64Array, index: number, vector: Vector3Dd): void {
        values[index * 3] = vector.x();
        values[index * 3 + 1] = vector.y();
        values[index * 3 + 2] = vector.z();
    }
    private setPositionAndNormal(index: number, vertex: Vertex): void {
        this.setVector(this.vertexPositions!, index, vertex.position);
        this.setVector(this.vertexNormals!, index, vertex.normal);
    }
    private quad(index: number): [number, number, number, number] {
        if (this.quadIndices === null || index < 0 || index >= this.getNumQuads())
            throw new RangeError("Quad index out of bounds");
        const i = index * 4;
        return [this.quadIndices[i]!, this.quadIndices[i + 1]!, this.quadIndices[i + 2]!, this.quadIndices[i + 3]!];
    }
}
