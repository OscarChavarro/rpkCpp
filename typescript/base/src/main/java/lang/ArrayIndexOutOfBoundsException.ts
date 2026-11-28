import { IndexOutOfBoundsException } from "./IndexOutOfBoundsException.js";
import type { Throwable } from "./Throwable.js";

export class ArrayIndexOutOfBoundsException extends IndexOutOfBoundsException {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "ArrayIndexOutOfBoundsException";
    }
}
