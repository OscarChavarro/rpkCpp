import { Numeric } from "./Numeric.js";
import { Vector3D } from "./Vector3D.js";

export class CoordinateSystem {
    private x: Vector3D = new Vector3D();
    private y: Vector3D = new Vector3D();
    private z: Vector3D = new Vector3D();

    public getX(): Vector3D {
        return this.x;
    }

    public getY(): Vector3D {
        return this.y;
    }

    public getZ(): Vector3D {
        return this.z;
    }

    public setX(inX: Vector3D): void {
        this.x.copy(inX);
    }

    public setY(inY: Vector3D): void {
        this.y.copy(inY);
    }

    public setZ(inZ: Vector3D): void {
        this.z.copy(inZ);
    }

    public setFromZAxis(inZ: Vector3D): void {
        this.z.copy(inZ);

        const zz = globalThis.Math.sqrt(1.0 - inZ.z * inZ.z);
        if (zz < Numeric.EPSILON) {
            this.x.x = 1.0;
            this.x.y = 0.0;
            this.x.z = 0.0;
        } else {
            this.x.x = inZ.y / zz;
            this.x.y = -inZ.x / zz;
            this.x.z = 0.0;
        }

        this.y.crossProduct(this.z, this.x);
    }

    public rectangularToSphericalCoord(cIn: Vector3D, phi: number[], theta: number[]): void {
        if (phi === null || phi.length === 0 || theta === null || theta.length === 0) {
            throw new Error("phi/theta output arrays must have length >= 1");
        }

        let z = cIn.dotProduct(this.z);
        if (z > 1.0) {
            z = 1.0;
        }
        if (z < -1.0) {
            z = -1.0;
        }

        theta[0] = globalThis.Math.acos(z);

        const c = new Vector3D();
        c.sumScaled(cIn, -z, this.z);
        c.normalize(Numeric.EPSILON_FLOAT);

        let x = c.dotProduct(this.x);
        const y = c.dotProduct(this.y);

        if (x > 1.0) {
            x = 1.0;
        }
        if (x < -1.0) {
            x = -1.0;
        }

        phi[0] = globalThis.Math.acos(x);
        if (y < 0.0) {
            phi[0] = 2.0 * globalThis.Math.PI - phi[0];
        }
    }

    public sampleHemisphereCosTheta(xi1: number, xi2: number, probabilityDensityFunction: number[] | null): Vector3D {
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.sqrt(1.0 - xi2);
        const sinTheta = globalThis.Math.sqrt(xi2);

        const dir = new Vector3D();
        dir.combine(cosPhi, this.x, sinPhi, this.y);
        dir.combine(sinTheta, dir, cosTheta, this.z);

        if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = cosTheta / globalThis.Math.PI;
        }

        return dir;
    }

    public sampleHemisphereCosNTheta(
        n: number,
        xi1: number,
        xi2: number,
        probabilityDensityFunction: number[] | null,
    ): Vector3D {
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.pow(xi2, 1.0 / (n + 1.0));
        const sinTheta = globalThis.Math.sqrt(1.0 - cosTheta * cosTheta);

        const dir = new Vector3D();
        dir.combine(cosPhi, this.x, sinPhi, this.y);
        dir.combine(sinTheta, dir, cosTheta, this.z);

        if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ((n + 1.0) * globalThis.Math.pow(cosTheta, n)) / (2.0 * globalThis.Math.PI);
        }

        return dir;
    }
}
