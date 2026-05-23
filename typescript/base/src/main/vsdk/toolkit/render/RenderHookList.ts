import { RenderHook } from "./RenderHook";

export class RenderHookList {
  private static renderHookList: RenderHook[] | null = [];

  private constructor() {
  }

  public static renderHooks(): void {
    for (let i = 0; RenderHookList.renderHookList !== null && i < RenderHookList.renderHookList.length; i++) {
      const h = RenderHookList.renderHookList[i]!;
      if (h.function !== null) {
        h.function.apply(h.data);
      }
    }
  }

  public static removeAllRenderHooks(): void {
    RenderHookList.renderHookList = null;
  }
}
