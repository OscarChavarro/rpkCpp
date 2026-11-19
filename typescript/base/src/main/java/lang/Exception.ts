import { Throwable } from "./Throwable.js";

export class Exception extends Throwable {
    public constructor(message?: string, cause?: Throwable) {
        super(message, cause);
        this.name = "Exception";
    }
}
