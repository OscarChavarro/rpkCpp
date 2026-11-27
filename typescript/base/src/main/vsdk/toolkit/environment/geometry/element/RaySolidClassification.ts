import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { RayBoundary } from "./RayBoundary.js";
import type { RaySolidBoundary } from "./RaySolidBoundary.js";

// For N ordered boundaries there are N+1 segment membership values. Endpoint
// events are included; a zero-length endpoint segment has the limiting state
// just outside the query interval. Point tangencies do not create solid segments.
// On failure both arrays are empty. Clear retains storage for query/worker reuse.
export class RaySolidClassification {
    public readonly boundaries = new ArrayList<RaySolidBoundary>();
    public readonly insideSegments = new ArrayList<boolean>();

    public constructor();
    /** C++ copy constructor. */
    public constructor(other: RaySolidClassification);
    public constructor(other?: RaySolidClassification) {
        if (other !== undefined) this.assign(other);
    }
    public clear(): void {
        this.boundaries.clear();
        this.insideSegments.clear();
    }
    /** C++ copy assignment. */
    public assign(other: RaySolidClassification): RaySolidClassification {
        // Reuses the existing storage, see RayInstancePath.
        if (this !== other) {
            this.boundaries.clear();
            for (let i = 0; i < other.boundaries.size(); i++)
                this.boundaries.add(new RayBoundary(other.boundaries.get(i)));
            this.insideSegments.clear();
            for (let i = 0; i < other.insideSegments.size(); i++) this.insideSegments.add(other.insideSegments.get(i));
        }
        return this;
    }
}
