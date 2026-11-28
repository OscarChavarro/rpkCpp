import { IOException } from "./IOException.js";

/** `java.io.EOFException`: end of file or end of stream reached unexpectedly. */
export class EOFException extends IOException {
    public constructor(message?: string) {
        super(message);
        this.name = "EOFException";
    }
}
