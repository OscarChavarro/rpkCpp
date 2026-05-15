import { RenderHookFunction } from "./RenderHookFunction";

export class RenderHook {
  public function: RenderHookFunction | null;
  public data: unknown;

  public constructor() {
    this.function = null;
    this.data = null;
  }
}
