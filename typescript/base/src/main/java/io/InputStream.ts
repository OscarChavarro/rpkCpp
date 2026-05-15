export abstract class InputStream {
  public abstract read(): number;
  public abstract read(buffer: Uint8Array, offset: number, length: number): number;
  public abstract read(buffer?: Uint8Array, offset?: number, length?: number): number;

  public abstract close(): void;

  public dispose(): void {
    this.close();
  }
}
