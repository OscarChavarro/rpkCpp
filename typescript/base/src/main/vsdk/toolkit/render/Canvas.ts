import { Logger } from "../common/logging/Logger";

export class Canvas {
  private static readonly CANVAS_MODE_STACK_SIZE = 5;
  private static modeStackIndex = 0;

  private constructor() {
  }

  public static canvasPushMode(): void {
    Canvas.modeStackIndex++;
    if (Canvas.modeStackIndex >= Canvas.CANVAS_MODE_STACK_SIZE) {
      Logger.fatal(4, "canvasPushMode", "Mode stack size (%d) exceeded.", Canvas.CANVAS_MODE_STACK_SIZE);
    }
  }

  public static canvasPullMode(): void {
    Canvas.modeStackIndex--;
    if (Canvas.modeStackIndex < 0) {
      Logger.fatal(4, "canvasPullMode", "Canvas mode stack underflow.\n");
    }
  }
}
