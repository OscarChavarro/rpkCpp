import type { Entity } from "../../../common/Entity.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";

// Identity is borrowed; transforms are owned snapshots from child to parent and back.
export class RayInstanceStep {
    public instance: Entity | null;
    public childToParent: Matrix4x4d;
    public parentToChild: Matrix4x4d;
    public constructor();
    public constructor(instance: Entity | null, childToParent: Matrix4x4d, parentToChild: Matrix4x4d);
    /** C++ copy constructor. */
    public constructor(other: RayInstanceStep);
    public constructor(value?: Entity | RayInstanceStep | null, forward?: Matrix4x4d, inverse?: Matrix4x4d) {
        if (value instanceof RayInstanceStep) {
            this.instance = value.instance;
            this.childToParent = value.childToParent;
            this.parentToChild = value.parentToChild;
            return;
        }
        this.instance = value ?? null;
        this.childToParent = forward ?? new Matrix4x4d();
        this.parentToChild = inverse ?? new Matrix4x4d();
    }
    /** C++ copy assignment; matrices are immutable in this port. */
    public assign(other: RayInstanceStep): RayInstanceStep {
        this.instance = other.instance;
        this.childToParent = other.childToParent;
        this.parentToChild = other.parentToChild;
        return this;
    }
}
