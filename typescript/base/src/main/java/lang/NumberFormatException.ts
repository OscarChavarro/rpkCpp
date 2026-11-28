import { IllegalArgumentException } from "./IllegalArgumentException.js";
import { Throwable } from "./Throwable.js";

/** Java's `NumberFormatException`, raised by the text to number parsers. */
export class NumberFormatException extends IllegalArgumentException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "NumberFormatException";
    }
}
