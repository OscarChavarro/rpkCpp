import { InputStream } from "../../../../java/io/InputStream";

export class ReaderContext {
  public static readonly MGF_MAXIMUM_INPUT_LINE_LENGTH = 4096;
  public static readonly MGF_MAXIMUM_ARGUMENT_COUNT = (ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH / 4);

  public fileName: string;
  public inputStream: InputStream | null;
  public fileContextId: number;
  public inputLine: string;
  public lineNumber: number;
  public isPipe: number;
  public prev: ReaderContext | null;

  public constructor() {
    this.fileName = "";
    this.inputStream = null;
    this.fileContextId = 0;
    this.inputLine = "";
    this.lineNumber = 0;
    this.isPipe = 0;
    this.prev = null;
  }
}
