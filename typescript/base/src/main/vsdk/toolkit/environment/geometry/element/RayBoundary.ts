import type { Geometry } from "../Geometry.js";
import { RayInstancePath } from "./RayInstancePath.js";
import type { RayInstanceStep } from "./RayInstanceStep.js";

// Mathematical provenance only. Scene resolves appearance from the borrowed leaf.
export class RayBoundary {
    public t: number;
    public geometry: Geometry | null;
    public normalReversed: boolean;
    // Face/element within the leaf geometry, or -1 when not applicable.
    public surfaceIndex: number;
    // Ordered from the leaf instance outward. Values are snapshots, identities borrowed.
    private readonly path: RayInstancePath;

    public getProvenance(): RayInstancePath {
        return this.path;
    }
    public appendInstance(step: RayInstanceStep): boolean {
        return this.path.appendInstance(step);
    }
    public getInstanceCount(): number {
        return this.path.getInstanceCount();
    }
    public getInstance(index: number, result: RayInstanceStep): boolean {
        return this.path.getInstance(index, result);
    }
    public clearInstances(): void {
        this.path.clearInstances();
    }

    public constructor();
    /** C++ copy constructor. */
    public constructor(other: RayBoundary);
    public constructor(t: number, geometry: Geometry | null, normalReversed?: boolean, surfaceIndex?: number);
    public constructor(value?: number | RayBoundary, leaf: Geometry | null = null, reversed = false, index = -1) {
        if (value instanceof RayBoundary) {
            this.t = value.t;
            this.geometry = value.geometry;
            this.normalReversed = value.normalReversed;
            this.surfaceIndex = value.surfaceIndex;
            this.path = new RayInstancePath(value.path);
            return;
        }
        this.t = value ?? 0;
        this.geometry = leaf;
        this.normalReversed = reversed;
        this.surfaceIndex = index;
        this.path = new RayInstancePath();
    }
    /** C++ copy assignment. */
    public assign(other: RayBoundary): RayBoundary {
        if (this !== other) {
            this.t = other.t;
            this.geometry = other.geometry;
            this.normalReversed = other.normalReversed;
            this.surfaceIndex = other.surfaceIndex;
            this.path.assign(other.path);
        }
        return this;
    }
}
