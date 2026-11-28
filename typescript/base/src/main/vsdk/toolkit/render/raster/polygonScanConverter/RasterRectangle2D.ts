/**
Immutable integer screen rectangle. The minimum corner (minX, minY) and the
maximum corner (maxX, maxY) are both inclusive. Instances offer no mutators: a
new value is obtained by constructing another rectangle.
*/
export class RasterRectangle2D {
    private readonly minX: number;
    private readonly minY: number;
    private readonly maxX: number;
    private readonly maxY: number;

    public constructor();
    public constructor(inMinX: number, inMinY: number, inMaxX: number, inMaxY: number);
    public constructor(inMinX = 0, inMinY = 0, inMaxX = 0, inMaxY = 0) {
        this.minX = inMinX;
        this.minY = inMinY;
        this.maxX = inMaxX;
        this.maxY = inMaxY;
    }

    public getMinX(): number {
        return this.minX;
    }

    public getMinY(): number {
        return this.minY;
    }

    public getMaxX(): number {
        return this.maxX;
    }

    public getMaxY(): number {
        return this.maxY;
    }
}
