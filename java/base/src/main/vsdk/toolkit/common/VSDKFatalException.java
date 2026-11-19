package vsdk.toolkit.common;
import java.io.Serial;

/**
Unchecked exception raised when VSDK reports a fatal error in non-exit mode.
*/
public class VSDKFatalException extends RuntimeException {
    @Serial private static final long serialVersionUID = 20260417L;

    public VSDKFatalException(String message) {
        super(message);
    }

    public VSDKFatalException(String message, Throwable cause) {
        super(message, cause);
    }
}
