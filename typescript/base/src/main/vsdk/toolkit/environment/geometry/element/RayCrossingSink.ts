import type { RayCrossing } from "./RayCrossing.js";
export interface RayCrossingSink {
    crossing(event: RayCrossing): void;
}
