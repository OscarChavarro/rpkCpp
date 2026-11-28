/**
Java's `java.lang.Cloneable` marker interface. TypeScript interfaces leave
nothing at run time, so a class that implements `Cloneable` in Java also
declares the static marker

    public static readonly [CLONEABLE] = true;

which its subclasses inherit, as they inherit the Java interface.
`Object.clone()` checks it through `isCloneable`.
*/
export interface Cloneable {}

export const CLONEABLE: unique symbol = Symbol("java.lang.Cloneable");

/** @return true if the class of `value` implements Java's `Cloneable` */
export function isCloneable(value: object): boolean {
    return (value.constructor as unknown as Record<symbol, unknown>)[CLONEABLE] === true;
}
