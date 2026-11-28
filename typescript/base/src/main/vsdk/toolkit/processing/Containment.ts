export enum Containment {
    OUTSIDE = -1,
    LIMIT = 0,
    INSIDE = 1,
}

/** Java's `Containment.value()`: the integer code of the constant. */
export function containmentValue(containment: Containment): number {
    return containment;
}
