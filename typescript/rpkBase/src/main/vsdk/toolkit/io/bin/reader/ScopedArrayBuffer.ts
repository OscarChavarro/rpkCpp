export class ScopedArrayBuffer<T> {
  private value: T | null;

  public constructor(initialValue: T | null = null) {
    this.value = initialValue;
  }

  public reset(newValue: T | null): void {
    this.value = newValue;
  }

  public get(): T | null {
    return this.value;
  }
}
