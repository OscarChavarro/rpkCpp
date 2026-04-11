export class FilePositionContext {
  public fileId: number;
  public lineNumber: number;
  public offset: number;

  public constructor() {
    this.fileId = 0;
    this.lineNumber = 0;
    this.offset = 0;
  }
}
