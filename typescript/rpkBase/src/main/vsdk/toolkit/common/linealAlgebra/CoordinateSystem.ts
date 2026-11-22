import { Numeric } from "./Numeric.js";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class CoordinateSystem {
    private x: Vector3Df = new Vector3Df();
    private y: Vector3Df = new Vector3Df();
    private z: Vector3Df = new Vector3Df();

    public getX(): Vector3Df {
        return this.x;
    }

    public getY(): Vector3Df {
        return this.y;
    }

    public getZ(): Vector3Df {
        return this.z;
    }

    public setX(inX: Vector3Df): void {
        this.x = inX;
    }

    public setY(inY: Vector3Df): void {
        this.y = inY;
    }

    public setZ(inZ: Vector3Df): void {
        this.z = inZ;
    }

    public setFromZAxis(inZ: Vector3Df): void {
        this.z = inZ;

        const zz = globalThis.Math.sqrt(1.0 - inZ.z() * inZ.z());
        if (zz < VSDK.EPSILON) {
            this.x = this.x.withX(1.0);
            this.x = this.x.withY(0.0);
            this.x = this.x.withZ(0.0);
        } else {
            this.x = this.x.withX(inZ.y() / zz);
            this.x = this.x.withY(-inZ.x() / zz);
            this.x = this.x.withZ(0.0);
        }

        this.y = this.z.crossProduct(this.x);
    }

    public rectangularToSphericalCoord(cIn: Vector3Df, phi: number[], theta: number[]): void {
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

        let c = new Vector3Df();
        c = cIn.add(this.z.multiply(-z));
        c = c.normalized(Numeric.EPSILON_FLOAT);

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

    public sampleHemisphereCosTheta(xi1: number, xi2: number, probabilityDensityFunction: number[] | null): Vector3Df {
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.sqrt(1.0 - xi2);
        const sinTheta = globalThis.Math.sqrt(xi2);

        let dir = new Vector3Df();
        dir = this.x.multiply(cosPhi).add(this.y.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(this.z.multiply(cosTheta));

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
    ): Vector3Df {
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.pow(xi2, 1.0 / (n + 1.0));
        const sinTheta = globalThis.Math.sqrt(1.0 - cosTheta * cosTheta);

        let dir = new Vector3Df();
        dir = this.x.multiply(cosPhi).add(this.y.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(this.z.multiply(cosTheta));

        if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ((n + 1.0) * globalThis.Math.pow(cosTheta, n)) / (2.0 * globalThis.Math.PI);
        }

        return dir;
    }
}
