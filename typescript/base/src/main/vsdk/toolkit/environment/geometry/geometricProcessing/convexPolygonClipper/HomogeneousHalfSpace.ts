/**
Immutable clipping half-space in homogeneous coordinates: the vertices with
a * sx + b * sy + c * sz + d * sw <= 0 are inside, where (a, b, c, d) are the
x, y, z and w coefficients. A convex window is the intersection of such
half-spaces ([SUTH1974b], p. 33). For example, the screen box side x >= xMin
is (-1, 0, 0, xMin) and x <= xMax is (1, 0, 0, -xMax).
*/
export class HomogeneousHalfSpace {
    private readonly xCoefficient: number;
    private readonly yCoefficient: number;
    private readonly zCoefficient: number;
    private readonly wCoefficient: number;

    public constructor();
    public constructor(inXCoefficient: number, inYCoefficient: number, inZCoefficient: number, inWCoefficient: number);
    public constructor(inXCoefficient = 0.0, inYCoefficient = 0.0, inZCoefficient = 0.0, inWCoefficient = 0.0) {
        this.xCoefficient = inXCoefficient;
        this.yCoefficient = inYCoefficient;
        this.zCoefficient = inZCoefficient;
        this.wCoefficient = inWCoefficient;
    }

    public getXCoefficient(): number {
        return this.xCoefficient;
    }

    public getYCoefficient(): number {
        return this.yCoefficient;
    }

    public getZCoefficient(): number {
        return this.zCoefficient;
    }

    public getWCoefficient(): number {
        return this.wCoefficient;
    }
}
