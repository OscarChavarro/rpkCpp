/** Immutable box, typically in screen space */
export class PolygonBox {
    private readonly minX: number; // Left and right
    private readonly maxX: number;
    private readonly minY: number; // Top and bottom
    private readonly maxY: number;
    private readonly minZ: number; // Near and far
    private readonly maxZ: number;

    public constructor(inMinX: number, inMaxX: number, inMinY: number, inMaxY: number, inMinZ: number, inMaxZ: number) {
        this.minX = inMinX;
        this.maxX = inMaxX;
        this.minY = inMinY;
        this.maxY = inMaxY;
        this.minZ = inMinZ;
        this.maxZ = inMaxZ;
    }

    public getMinX(): number {
        return this.minX;
    }

    public getMaxX(): number {
        return this.maxX;
    }

    public getMinY(): number {
        return this.minY;
    }

    public getMaxY(): number {
        return this.maxY;
    }

    public getMinZ(): number {
        return this.minZ;
    }

    public getMaxZ(): number {
        return this.maxZ;
    }
}
