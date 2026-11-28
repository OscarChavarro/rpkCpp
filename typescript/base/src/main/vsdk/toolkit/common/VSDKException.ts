import { Exception } from "../../../java/lang/Exception.js";
import { Throwable } from "../../../java/lang/Throwable.js";

/** Checked-exception base for Vitral-specific failures. */
export abstract class VSDKException extends Exception {
    public constructor();
    public constructor(message: string);
    public constructor(cause: Throwable);
    public constructor(message: string, cause: Throwable);
    public constructor(messageOrCause?: string | Throwable, cause?: Throwable) {
        if (typeof messageOrCause === "string") super(messageOrCause, cause);
        else if (messageOrCause !== undefined) super(undefined, messageOrCause);
        else super();
        this.name = "VSDKException";
    }
}
