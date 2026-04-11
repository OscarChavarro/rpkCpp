export class LookUpEntity<T> {
  public key: string | null;
  public value: number;
  public data: T | null;

  public constructor() {
    this.key = null;
    this.value = 0;
    this.data = null;
  }
}
