import { Exception } from "./Exception.js";
import { Throwable } from "./Throwable.js";

export class CloneNotSupportedException extends Exception {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "CloneNotSupportedException";
    }
}
