import { RuntimeException } from "./RuntimeException.js";
import { Throwable } from "./Throwable.js";

export class UnsupportedOperationException extends RuntimeException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "UnsupportedOperationException";
    }
}
