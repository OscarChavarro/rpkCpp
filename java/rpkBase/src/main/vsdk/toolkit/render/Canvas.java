package vsdk.toolkit.render;

import vsdk.toolkit.common.logging.Logger;

public class Canvas {
    private static final int CANVAS_MODE_STACK_SIZE = 5;
    private static int modeStackIndex = 0;

    /**
Pushes the current canvas mode on the canvas mode stack, so it can be restored later
*/
    public static void canvasPushMode() {
        modeStackIndex++;
        if (modeStackIndex >= CANVAS_MODE_STACK_SIZE) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "canvasPushMode", String.format("Mode stack size (%d) exceeded.", CANVAS_MODE_STACK_SIZE));
        }
    }

    /**
Restores the last saved canvas mode
*/
    public static void canvasPullMode() {
        modeStackIndex--;
        if (modeStackIndex < 0) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "canvasPullMode", "Canvas mode stack underflow.\n");
        }
    }
}
