import { Vector3Dd } from "./Vector3Dd.js";

/**
Routines used to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437

References:
- [ARVO1995b] "Stratified Sampling of Spherical Triangles", SIGGRAPH 1995

This class implements local-frame and spherical-coordinate primitives after
Section [ARVO1995b].2, especially the orthogonal normalized component operator from
equation (6). Equations (1)-(5) in the paper concern spherical
triangle area inversion and are not directly implemented here.

The methods that compute more than one value write them in one-element
arrays given as output holders.
*/
export class CoordinateSystem {
    private xAxis: Vector3Dd = new Vector3Dd();
    private yAxis: Vector3Dd = new Vector3Dd();
    private zAxis: Vector3Dd = new Vector3Dd();

    public getXAxis(): Vector3Dd {
        return this.xAxis;
    }

    public getYAxis(): Vector3Dd {
        return this.yAxis;
    }

    public getZAxis(): Vector3Dd {
        return this.zAxis;
    }

    public setXAxis(inX: Vector3Dd): void {
        this.xAxis = inX;
    }

    public setYAxis(inY: Vector3Dd): void {
        this.yAxis = inY;
    }

    public setZAxis(inZ: Vector3Dd): void {
        this.zAxis = inZ;
    }

    /**
    Creates a coordinate system with the given UNIT direction vector as inZ-axis.
    When inZ is within epsilon of the world Z-axis, world X is taken as X-axis
    */
    public setFromZAxis(inZ: Vector3Dd, epsilon: number): void {
        this.zAxis = inZ;

        // Equation [ARVO1995b](6): [x|y] = Normalize(x - (x.y) y).
        // X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).
        const zz = globalThis.Math.sqrt(1.0 - inZ.z() * inZ.z());
        if (zz < epsilon) {
            this.xAxis = this.xAxis.withX(1.0);
            this.xAxis = this.xAxis.withY(0.0);
            this.xAxis = this.xAxis.withZ(0.0);
        } else {
            this.xAxis = this.xAxis.withX(inZ.y() / zz);
            this.xAxis = this.xAxis.withY(-inZ.x() / zz);
            this.xAxis = this.xAxis.withZ(0.0);
        }

        // Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.
        this.yAxis = this.zAxis.crossProduct(this.xAxis);
    }

    /**
    Given a unit vector and a coordinate system, this routine computes the spherical
    coordinates phi and theta of the vector with respect to the coordinate system.
    epsilon is the length tolerance used to normalize the projection of C onto the
    XY plane
    */
    public rectangularToSphericalCoord(cIn: Vector3Dd, phi: number[], theta: number[], epsilon: number): void {
        if (phi === null || phi.length === 0 || theta === null || theta.length === 0) {
            throw new Error("phi/theta output arrays must have length >= 1");
        }

        let z = cIn.dotProduct(this.zAxis);
        if (z > 1.0) {
            z = 1.0;
        }
        // Sometimes numerical errors cause this
        if (z < -1.0) {
            z = -1.0;
        }

        theta[0] = globalThis.Math.acos(z);

        // Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C.Z) Z).
        let c = new Vector3Dd();
        c = cIn.add(this.zAxis.multiply(-z));
        c = c.normalized(epsilon);

        let x = c.dotProduct(this.xAxis);
        const y = c.dotProduct(this.yAxis);

        if (x > 1.0) {
            x = 1.0;
        }
        // Sometimes numerical errors cause this
        if (x < -1.0) {
            x = -1.0;
        }

        phi[0] = globalThis.Math.acos(x);
        if (y < 0.0) {
            phi[0] = 2.0 * globalThis.Math.PI - phi[0];
        }
    }

    /**
    Samples the hemisphere according to a cos_theta distribution
    */
    public sampleHemisphereCosTheta(xi1: number, xi2: number, probabilityDensityFunction: number[] | null): Vector3Dd {
        // Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.sqrt(1.0 - xi2);
        const sinTheta = globalThis.Math.sqrt(xi2);

        let dir = new Vector3Dd();
        dir = this.xAxis.multiply(cosPhi).add(this.yAxis.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(this.zAxis.multiply(cosTheta));

        if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = cosTheta / globalThis.Math.PI;
        }

        return dir;
    }

    /**
    Samples the hemisphere according to a cos_theta ^ n distribution
    */
    public sampleHemisphereCosNTheta(
        n: number,
        xi1: number,
        xi2: number,
        probabilityDensityFunction: number[] | null,
    ): Vector3Dd {
        // Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.
        const phi = 2.0 * globalThis.Math.PI * xi1;
        const cosPhi = globalThis.Math.cos(phi);
        const sinPhi = globalThis.Math.sin(phi);
        const cosTheta = globalThis.Math.pow(xi2, 1.0 / (n + 1.0));
        const sinTheta = globalThis.Math.sqrt(1.0 - cosTheta * cosTheta);

        let dir = new Vector3Dd();
        dir = this.xAxis.multiply(cosPhi).add(this.yAxis.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(this.zAxis.multiply(cosTheta));

        if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ((n + 1.0) * globalThis.Math.pow(cosTheta, n)) / (2.0 * globalThis.Math.PI);
        }

        return dir;
    }
}
