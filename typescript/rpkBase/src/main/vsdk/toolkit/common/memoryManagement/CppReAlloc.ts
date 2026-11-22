export class CppReAlloc {
  private constructor() {
  }

  public static reAlloc(ptr: Uint8Array | null, oldElementCount: number, newElementCount: number): Uint8Array | null;
  public static reAlloc(ptr: string[][] | null, oldElementCount: number, newElementCount: number): string[][] | null;
  public static reAlloc(
    ptr: Uint8Array | string[][] | null,
    oldElementCount: number,
    newElementCount: number
  ): Uint8Array | string[][] | null {
    if (newElementCount <= 0) {
      return null;
    }

    if (ptr instanceof Uint8Array || ptr === null) {
      const newPtr = new Uint8Array(newElementCount);
      if (ptr !== null && oldElementCount > 0) {
        const copyElementCount = globalThis.Math.min(oldElementCount, newElementCount);
        newPtr.set(ptr.slice(0, copyElementCount), 0);
      }
      return newPtr;
    }

    const newPtr: string[][] = new Array<string[]>(newElementCount);
    for (let i = 0; i < newElementCount; i++) {
      newPtr[i] = [];
    }

    if (ptr !== null && oldElementCount > 0) {
      const copyElementCount = globalThis.Math.min(oldElementCount, newElementCount);
      for (let i = 0; i < copyElementCount; i++) {
        newPtr[i] = ptr[i] ?? [];
      }
    }
    return newPtr;
  }
}
