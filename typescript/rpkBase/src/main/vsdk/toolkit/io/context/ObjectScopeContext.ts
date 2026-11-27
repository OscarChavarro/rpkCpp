import { ParseErrorContext } from "./ParseErrorContext";

export class ObjectScopeContext {
  private readonly objectNamesList: string[];

  public constructor() {
    this.objectNamesList = [];
  }

  public destroy(): void {
    this.clear();
  }

  public pushName(name: string | null): number {
    if (name === null) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    this.objectNamesList.push(name);
    return ParseErrorContext.MGF_OK;
  }

  public popName(): number {
    const count = this.objectNamesList.length;
    if (count < 1) {
      return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
    }
    this.objectNamesList.splice(count - 1, 1);
    return ParseErrorContext.MGF_OK;
  }

  public clear(): void {
    this.objectNamesList.length = 0;
  }
}
