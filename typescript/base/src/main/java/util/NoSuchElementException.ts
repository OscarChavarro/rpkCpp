import { RuntimeException } from "../lang/RuntimeException.js";
import { Throwable } from "../lang/Throwable.js";

export class NoSuchElementException extends RuntimeException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "NoSuchElementException";
    }
}
