import { NullPointerException } from "../lang/NullPointerException.js";

/**
Subset of `java.util.Objects` needed by ported classes whose contracts depend
on Java's null-checking behavior and messages.
*/
export class Objects {
    private constructor() {}

    /**
    `Objects.requireNonNull(T)` and `Objects.requireNonNull(T, String)`:
    returns the given reference, or throws `NullPointerException` with the
    given message when it is `null`.
    */
    public static requireNonNull<T>(obj: T | null | undefined, message?: string): T {
        if (obj === null || obj === undefined) {
            throw new NullPointerException(message);
        }
        return obj;
    }
}
