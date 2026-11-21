//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

/**
Axis aligned box of the build. The default box is the "negative box"
[+inf, -inf] of [WALD2007].3.4: enclosing it in another box leaves that box
unchanged, so bins and partitions grow without emptiness checks.
*/
export class _BinnedSahBox {
    public readonly minimum: number[] = [0, 0, 0];
    public readonly maximum: number[] = [0, 0, 0];

    public constructor() {
        for (let a = 0; a < 3; a++) {
            this.minimum[a] = 1e30;
            this.maximum[a] = -1e30;
        }
    }

    public isEmpty(): boolean {
        return (
            this.minimum[0]! > this.maximum[0]! ||
            this.minimum[1]! > this.maximum[1]! ||
            this.minimum[2]! > this.maximum[2]!
        );
    }

    public enclose(other: _BinnedSahBox): void {
        for (let a = 0; a < 3; a++) {
            this.minimum[a] = other.minimum[a]! < this.minimum[a]! ? other.minimum[a]! : this.minimum[a]!;
            this.maximum[a] = other.maximum[a]! > this.maximum[a]! ? other.maximum[a]! : this.maximum[a]!;
        }
    }

    public enclosePoint(point: number[]): void {
        for (let a = 0; a < 3; a++) {
            this.minimum[a] = point[a]! < this.minimum[a]! ? point[a]! : this.minimum[a]!;
            this.maximum[a] = point[a]! > this.maximum[a]! ? point[a]! : this.maximum[a]!;
        }
    }

    public centroid(axis: number): number {
        return 0.5 * (this.minimum[axis]! + this.maximum[axis]!);
    }

    public extent(axis: number): number {
        return this.maximum[axis]! - this.minimum[axis]!;
    }

    /** Half the surface area: the SAH cost of [WALD2007].3.1 drops the
    constant factors and only compares ratios */
    public halfArea(): number {
        if (this.isEmpty()) {
            return 0.0;
        }
        const dx = this.maximum[0]! - this.minimum[0]!;
        const dy = this.maximum[1]! - this.minimum[1]!;
        const dz = this.maximum[2]! - this.minimum[2]!;
        return dx * dy + dy * dz + dz * dx;
    }
}
