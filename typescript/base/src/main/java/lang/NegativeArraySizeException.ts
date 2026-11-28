import { RuntimeException } from "./RuntimeException.js";
import { Throwable } from "./Throwable.js";

/**
Java's `java.lang.NegativeArraySizeException`, thrown by `new T[n]` with a
negative `n` (the JDK message is the size, as in "-12").
*/
export class NegativeArraySizeException extends RuntimeException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "NegativeArraySizeException";
    }
}
