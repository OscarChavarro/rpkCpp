import { ParseErrorContext } from "./ParseErrorContext";

export class ObjectScopeContext {
  public objectNamesList: Array<string | null> | null;
  public objectMaxName: number;
  public objectNames: number;

  private static readonly OBJECT_NAMES_ALLOC_INCREMENT = 16;

  public constructor() {
    this.objectNamesList = null;
    this.objectMaxName = 0;
    this.objectNames = 0;
  }

  public destroy(): void {
    this.clear();
  }

  public pushName(name: string | null): number {
    if (name === null) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    try {
      if (this.objectNames >= this.objectMaxName - 1) {
        if (this.objectMaxName === 0) {
          this.objectMaxName = ObjectScopeContext.OBJECT_NAMES_ALLOC_INCREMENT;
          this.objectNamesList = new Array<string | null>(this.objectMaxName).fill(null);
        }
        else {
          const previousMaxName = this.objectMaxName;
          this.objectMaxName += ObjectScopeContext.OBJECT_NAMES_ALLOC_INCREMENT;
          const newNames = new Array<string | null>(this.objectMaxName).fill(null);
          for (let i = 0; this.objectNamesList !== null && i < this.objectNamesList.length && i < newNames.length; i++) {
            newNames[i] = this.objectNamesList[i] ?? null;
          }
          this.objectNamesList = newNames;
          if (this.objectNamesList === null) {
            this.objectMaxName = previousMaxName;
          }
        }
        if (this.objectNamesList === null) {
          this.objectMaxName = 0;
          this.objectNames = 0;
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
      }

      (this.objectNamesList as Array<string | null>)[this.objectNames] = name;
      this.objectNames++;
      if (this.objectNames < (this.objectNamesList as Array<string | null>).length) {
        (this.objectNamesList as Array<string | null>)[this.objectNames] = null;
      }
      return ParseErrorContext.MGF_OK;
    }
    catch (_error) {
      this.objectMaxName = 0;
      this.objectNames = 0;
      this.objectNamesList = null;
      return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
    }
  }

  public popName(): number {
    if (this.objectNames < 1) {
      return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
    }
    this.objectNames--;
    if (this.objectNamesList !== null) {
      this.objectNamesList[this.objectNames] = null;
    }
    return ParseErrorContext.MGF_OK;
  }

  public clear(): void {
    if (this.objectNamesList !== null) {
      for (let i = 0; i < this.objectNames; i++) {
        this.objectNamesList[i] = null;
      }
    }
    this.objectNamesList = null;
    this.objectMaxName = 0;
    this.objectNames = 0;
  }
}
