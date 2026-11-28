import { Exception } from "../lang/Exception.js";
import { Throwable } from "../lang/Throwable.js";

export class IOException extends Exception {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "IOException";
    }
}
