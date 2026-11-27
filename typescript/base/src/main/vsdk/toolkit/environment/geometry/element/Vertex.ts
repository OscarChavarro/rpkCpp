import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { VSDK } from "../../../common/VSDK.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Triangle } from "./Triangle.js";
export class Vertex extends FundamentalEntity {
    public position: Vector3Dd;
    public normal: Vector3Dd;
    public binormal: Vector3Dd;
    public tangent: Vector3Dd;
    public u = 0;
    public v = 0;
    public incidentTriangles: Triangle[] | null = null;
    public constructor(x: Vector3Dd | number, y?: Vector3Dd | number, z?: Vector3Dd | number, u?: number, v?: number) {
        super();
        if (typeof x === "number") {
            this.position = new Vector3Dd(x, y as number, z as number);
            this.normal = new Vector3Dd(1, 0, 0);
        } else {
            this.position = new Vector3Dd(x);
            this.normal = y instanceof Vector3Dd ? y.normalized() : new Vector3Dd(1, 0, 0);
            this.u = typeof u === "number" ? u : 0;
            this.v = typeof v === "number" ? v : 0;
        }
        this.binormal = new Vector3Dd(0, 1, 0);
        this.tangent = new Vector3Dd(0, 0, 1);
    }
    public getPosition() {
        return this.position;
    }
    public getNormal() {
        return this.normal;
    }
    public getBinormal() {
        return this.binormal;
    }
    public getTangent() {
        return this.tangent;
    }
    public getU() {
        return this.u;
    }
    public getV() {
        return this.v;
    }
    public getIncidentTriangles() {
        return this.incidentTriangles;
    }
    public setPosition(x: Vector3Dd) {
        this.position = new Vector3Dd(x);
    }
    public setNormal(x: Vector3Dd) {
        this.normal = x.normalized();
    }
    public setBinormal(x: Vector3Dd) {
        this.binormal = x;
    }
    public setTangent(x: Vector3Dd) {
        this.tangent = x;
    }
    public setU(x: number) {
        this.u = x;
    }
    public setV(x: number) {
        this.v = x;
    }
    public setIncidentTriangles(x: Triangle[] | null) {
        this.incidentTriangles = x;
    }
    public getIncidentTriangleAt(x: number) {
        return this.incidentTriangles![x]!;
    }
    public override toString() {
        return `v ${this.position} n ${this.normal} UV<${VSDK.formatDouble(this.u)}, ${VSDK.formatDouble(this.v)}>`;
    }
}
