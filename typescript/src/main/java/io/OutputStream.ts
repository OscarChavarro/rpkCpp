export abstract class OutputStream {
  public abstract write(value: number): void;
  public abstract write(buffer: Uint8Array, offset: number, length: number): void;
  public abstract write(valueOrBuffer: number | Uint8Array, offset?: number, length?: number): void;

  public flush(): void {
  }

  public abstract close(): void;

  public dispose(): void {
    this.close();
  }
}
