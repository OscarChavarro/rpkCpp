import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
export class Intersection {
    public constructor(
        private t: number,
        private point: Vector3Dd,
        private normal: Vector3Dd,
    ) {}
    public getT() {
        return this.t;
    }
    public setT(x: number) {
        this.t = x;
    }
    public getPoint() {
        return this.point;
    }
    public setPoint(x: Vector3Dd) {
        this.point = x;
    }
    public getNormal() {
        return this.normal;
    }
    public setNormal(x: Vector3Dd) {
        this.normal = x;
    }
}
