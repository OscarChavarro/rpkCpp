import { RuntimeException } from "../../../java/lang/RuntimeException.js";
import { Throwable } from "../../../java/lang/Throwable.js";

/** Unchecked failure reported by VSDK when fatal process exit is disabled. */
export class VSDKFatalException extends RuntimeException {
    public constructor(message: string, cause?: Throwable) {
        super(message, cause);
        this.name = "VSDKFatalException";
    }
}
