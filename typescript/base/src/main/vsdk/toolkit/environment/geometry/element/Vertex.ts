import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { VSDK } from "../../../common/VSDK.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { IndexOutOfBoundsException } from "../../../../../java/lang/IndexOutOfBoundsException.js";
import { NullPointerException } from "../../../../../java/lang/NullPointerException.js";
import type { Triangle } from "./Triangle.js";

/**
A vertex is a basic data pack for tipically used operations in computer
graphics, as polygon mesh representations and basic visualization algorithms
and shaders.

This class is meant as a modeling facility for polygon meshes, and a common
structure for operation signatures.

Note that this is NOT a class, but merely a data structure, and as such,
all its attributes are public.  This structure is not supposed to evolve or
change in time radically.

This class is supposed to be used as basic element for building polygons in
three dimensional space. Note that is being currently used in polygon meshes
(surfaces as such TriangleMesh) and on structured solid polygons, as such
BinarySpacePartitioningTreeSolid.

Java's `ArrayList<Triangle> incidentTriangles` is an array here;
`getIncidentTriangleAt` checks the index as `ArrayList.get` does.
*/
export class Vertex extends FundamentalEntity {
    ///
    public position: Vector3Dd;
    ///
    public normal: Vector3Dd;
    ///
    public binormal: Vector3Dd;
    ///
    public tangent: Vector3Dd;
    /// Texture coordinates
    public u: number;
    public v: number;
    ///
    public incidentTriangles: Triangle[] | null;

    public constructor(x: number, y: number, z: number);
    public constructor(position: Vector3Dd);
    public constructor(position: Vector3Dd, normal: Vector3Dd);
    public constructor(position: Vector3Dd, normal: Vector3Dd, u: number, v: number);
    public constructor(position: Vector3Dd, normal: Vector3Dd, binormal: Vector3Dd, tangent: Vector3Dd);
    public constructor(vertex: Vertex);
    public constructor(
        a: number | Vector3Dd | Vertex,
        b?: number | Vector3Dd,
        c?: number | Vector3Dd,
        d?: number | Vector3Dd,
    ) {
        super();
        if (typeof a === "number") {
            // Vertex(double x, double y, double z)
            this.position = new Vector3Dd(a, b as number, c as number);
            this.normal = new Vector3Dd(1, 0, 0);
            this.binormal = new Vector3Dd(0, 1, 0);
            this.tangent = new Vector3Dd(0, 0, 1);
            this.incidentTriangles = null;
            this.u = 0.0;
            this.v = 0.0;
        } else if (a instanceof Vertex) {
            // Vertex(Vertex vertex)
            const vertex: Vertex = a;
            this.position = vertex.position;
            this.normal = vertex.normal;
            this.binormal = vertex.binormal;
            this.tangent = vertex.tangent;
            this.incidentTriangles = vertex.getIncidentTriangles();
            this.u = vertex.u;
            this.v = vertex.v;
        } else if (b === undefined) {
            // Vertex(Vector3Dd position)
            this.position = a;
            this.normal = new Vector3Dd(1, 0, 0);
            this.binormal = new Vector3Dd(0, 1, 0);
            this.tangent = new Vector3Dd(0, 0, 1);
            this.incidentTriangles = null;
            this.u = 0.0;
            this.v = 0.0;
        } else if (c instanceof Vector3Dd) {
            // Vertex(Vector3Dd position, Vector3Dd normal, Vector3Dd binormal, Vector3Dd tangent)
            this.position = a;
            this.normal = b as Vector3Dd;
            this.binormal = c;
            this.tangent = d as Vector3Dd;
            this.incidentTriangles = null;
            this.u = 0.0;
            this.v = 0.0;
        } else {
            // Vertex(Vector3Dd position, Vector3Dd normal) and
            // Vertex(Vector3Dd position, Vector3Dd normal, double u, double v)
            this.position = new Vector3Dd(a);
            this.normal = new Vector3Dd(b as Vector3Dd).normalized();
            this.binormal = new Vector3Dd(0, 1, 0);
            this.tangent = new Vector3Dd(0, 0, 1);
            this.incidentTriangles = null;
            this.u = c === undefined ? 0.0 : c;
            this.v = c === undefined ? 0.0 : (d as number);
        }
    }

    public getPosition(): Vector3Dd {
        return this.position;
    }

    public getNormal(): Vector3Dd {
        return this.normal;
    }

    public getBinormal(): Vector3Dd {
        return this.binormal;
    }

    public getTangent(): Vector3Dd {
        return this.tangent;
    }

    public getU(): number {
        return this.u;
    }

    public getV(): number {
        return this.v;
    }

    public getIncidentTriangles(): Triangle[] | null {
        return this.incidentTriangles;
    }

    public setPosition(position: Vector3Dd): void {
        this.position = new Vector3Dd(position);
    }

    public setNormal(normal: Vector3Dd): void {
        this.normal = new Vector3Dd(normal).normalized();
    }

    public setBinormal(binormal: Vector3Dd): void {
        this.binormal = binormal;
    }

    public setTangent(tangent: Vector3Dd): void {
        this.tangent = tangent;
    }

    public setU(u: number): void {
        this.u = u;
    }

    public setV(v: number): void {
        this.v = v;
    }

    public setIncidentTriangles(incidentTriangles: Triangle[] | null): void {
        this.incidentTriangles = incidentTriangles;
    }

    public getIncidentTriangleAt(index: number): Triangle {
        if (this.incidentTriangles === null) {
            throw new NullPointerException();
        }
        if (index < 0 || index >= this.incidentTriangles.length) {
            throw new IndexOutOfBoundsException(
                "Index " + index + " out of bounds for length " + this.incidentTriangles.length,
            );
        }
        return this.incidentTriangles[index]!;
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable representation of current vertex
    */
    public override toString(): string {
        return (
            "v " +
            this.position.toString() +
            " n " +
            this.normal.toString() +
            " UV<" +
            VSDK.formatDouble(this.u) +
            ", " +
            VSDK.formatDouble(this.v) +
            ">"
        );
    }
}
