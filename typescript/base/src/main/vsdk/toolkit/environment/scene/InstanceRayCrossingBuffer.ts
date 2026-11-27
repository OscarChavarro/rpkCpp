import { ArrayList } from "../../../../java/util/ArrayList.js";
import type { Material } from "../material/Material.js";
import { RayBoundary } from "../geometry/element/RayBoundary.js";
import { RayCrossing } from "../geometry/element/RayCrossing.js";
import type { RayCrossingKind } from "../geometry/element/RayCrossingKind.js";
import type { RayCrossingSink } from "../geometry/element/RayCrossingSink.js";
import { RayInstanceStep } from "../geometry/element/RayInstanceStep.js";
import type { SceneSolid } from "./SceneSolid.js";

// Buffers events so a failed source query never emits a partial result.
export class InstanceRayCrossingBuffer implements RayCrossingSink {
    private readonly boundaries = new ArrayList<RayBoundary>();
    private readonly kinds = new ArrayList<RayCrossingKind>();
    private readonly bodies = new ArrayList<SceneSolid | null>();
    private readonly materials = new ArrayList<Material | null>();
    private readonly instance: RayInstanceStep;
    private readonly body: SceneSolid | null;
    private readonly material: Material | null;
    private failed: boolean;

    public constructor(value: RayInstanceStep, owner: SceneSolid | null, appearance: Material | null) {
        this.instance = new RayInstanceStep(value);
        this.body = owner;
        this.material = appearance;
        this.failed = false;
    }

    public crossing(event: RayCrossing): void {
        if (this.failed) return;
        const boundary = new RayBoundary(event);
        if (
            !boundary.appendInstance(this.instance) ||
            !this.boundaries.add(boundary) ||
            !this.kinds.add(event.kind) ||
            !this.bodies.add(event.body ? event.body : this.body) ||
            !this.materials.add(event.material ? event.material : this.material)
        )
            this.failed = true;
    }

    public isValid(): boolean {
        return !this.failed;
    }

    public deliver(sink: RayCrossingSink): void {
        if (this.failed) return;
        for (let i = 0; i < this.boundaries.size(); ++i) {
            const event = new RayCrossing(this.boundaries.get(i).t, this.boundaries.get(i).geometry, this.kinds.get(i));
            // static_cast<RayBoundary&>(event) = boundaries[i]
            RayBoundary.prototype.assign.call(event, this.boundaries.get(i));
            event.body = this.bodies.get(i);
            event.material = this.materials.get(i);
            sink.crossing(event);
        }
    }
}
