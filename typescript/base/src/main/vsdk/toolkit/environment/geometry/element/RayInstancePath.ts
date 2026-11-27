import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { RayInstanceStep } from "./RayInstanceStep.js";

// Surface locator and instance transforms, from leaf outward. Identities are borrowed.
export class RayInstancePath {
    private surfaceIndex: number;
    private readonly instances: ArrayList<RayInstanceStep>;
    public constructor();
    /** C++ copy constructor. */
    public constructor(other: RayInstancePath);
    public constructor(other?: RayInstancePath) {
        this.surfaceIndex = -1;
        this.instances = new ArrayList<RayInstanceStep>();
        if (other !== undefined) this.assign(other);
    }
    /** C++ copy assignment. */
    public assign(other: RayInstancePath): RayInstancePath {
        // Reuses the existing storage: assignment of query results must not
        // allocate once a workspace has reached its working size.
        if (this !== other) {
            this.surfaceIndex = other.surfaceIndex;
            this.instances.clear();
            for (let i = 0; i < other.instances.size(); i++)
                this.instances.add(new RayInstanceStep(other.instances.get(i)));
        }
        return this;
    }
    public setSurfaceIndex(index: number): boolean {
        if (index < -1) return false;
        this.surfaceIndex = index;
        return true;
    }
    public getSurfaceIndex(): number {
        return this.surfaceIndex;
    }
    public appendInstance(step: RayInstanceStep): boolean {
        return step.instance !== null && this.instances.add(new RayInstanceStep(step));
    }
    public getInstanceCount(): number {
        return this.instances.size();
    }
    public getInstance(index: number, result: RayInstanceStep): boolean {
        if (index < 0 || index >= this.instances.size()) return false;
        result.assign(this.instances.get(index));
        return true;
    }
    public clearInstances(): void {
        this.instances.clear();
    }
    public clear(): void {
        this.surfaceIndex = -1;
        this.instances.clear();
    }
}
