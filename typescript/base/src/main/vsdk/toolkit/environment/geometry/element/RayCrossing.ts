import type { Geometry } from "../Geometry.js";
import type { SceneSolid } from "../../scene/SceneSolid.js";
import type { Material } from "../../material/Material.js";
import type { RayCrossingKind } from "./RayCrossingKind.js";
import { RayBoundary } from "./RayBoundary.js";

// Leaf identity and optional instance/material attribution; pointers are borrowed.
export class RayCrossing extends RayBoundary {
    public kind: RayCrossingKind;
    public body: SceneSolid | null = null;
    public material: Material | null = null;
    public constructor(t: number, geometry: Geometry | null, kind: RayCrossingKind);
    /** C++ copy constructor. */
    public constructor(other: RayCrossing);
    public constructor(t: number | RayCrossing, geometry: Geometry | null = null, kind?: RayCrossingKind) {
        if (t instanceof RayCrossing) {
            super(t);
            this.kind = t.kind;
            this.body = t.body;
            this.material = t.material;
            return;
        }
        super(t, geometry);
        this.kind = kind!;
    }
    /** C++ copy assignment. */
    public override assign(other: RayBoundary): RayCrossing {
        super.assign(other);
        if (other instanceof RayCrossing) {
            this.kind = other.kind;
            this.body = other.body;
            this.material = other.material;
        }
        return this;
    }
}
