import { RuntimeException } from "./RuntimeException.js";
import { Throwable } from "./Throwable.js";

export class NullPointerException extends RuntimeException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "NullPointerException";
    }
}
