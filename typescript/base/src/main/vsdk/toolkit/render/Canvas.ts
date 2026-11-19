import { String as JavaString } from "../../../java/lang/String";
import { Logger } from "../common/logging/Logger";

export class Canvas {
  private static readonly CANVAS_MODE_STACK_SIZE = 5;
  private static modeStackIndex = 0;

  private constructor() {
  }

  public static canvasPushMode(): void {
    Canvas.modeStackIndex++;
    if (Canvas.modeStackIndex >= Canvas.CANVAS_MODE_STACK_SIZE) {
      Logger.reportMessage(null, Logger.FATAL_ERROR, "canvasPushMode", JavaString.vformat("Mode stack size (%d) exceeded.", [Canvas.CANVAS_MODE_STACK_SIZE]));
    }
  }

  public static canvasPullMode(): void {
    Canvas.modeStackIndex--;
    if (Canvas.modeStackIndex < 0) {
      Logger.reportMessage(null, Logger.FATAL_ERROR, "canvasPullMode", "Canvas mode stack underflow.\n");
    }
  }
}
