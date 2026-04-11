import { File } from "../../../../java/io/File";
import { FileOutputStream } from "../../../../java/io/FileOutputStream";
import { OutputStream } from "../../../../java/io/OutputStream";
import { ColorRgb } from "../../common/ColorRgb";
import { DkColor } from "./DkColor";
import { ImageOutputHandle } from "./ImageOutputHandle";

const util = require("node:util");

export class PicOutputHandle extends ImageOutputHandle {
  private outputStream: OutputStream | null;

  private static formatToString(format: string | null, ...argumentsList: unknown[]): string {
    if (format === null) {
      return "";
    }
    try {
      return util.format(format, ...argumentsList);
    }
    catch (_ignored) {
      return "";
    }
  }

  private static writeFormatted(
    outputStream: OutputStream | null,
    format: string | null,
    ...argumentsList: unknown[]
  ): void {
    if (outputStream === null || format === null) {
      return;
    }

    const text = PicOutputHandle.formatToString(format, ...argumentsList);
    if (text.length <= 0) {
      return;
    }

    const bytes = new Uint8Array(Buffer.from(text, "ascii"));
    try {
      outputStream.write(bytes, 0, bytes.length);
    }
    catch (_ignored) {
    }
  }

  private static formatCompileDate(date: Date): string {
    const monthNames = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    const month = monthNames[date.getMonth()];
    const day = `${date.getDate()}`.padStart(2, "0");
    return `${month} ${day} ${date.getFullYear()}`;
  }

  public constructor(filename: string, w: number, h: number) {
    super();
    this.init("high dynamic range PIC", w, h);
    this.outputStream = null;

    const file = new File(filename);
    const canWrite = file.canWrite();
    const isDirectory = file.isDirectory();
    file.dispose();

    if (!canWrite || isDirectory) {
      this.outputStream = null;
      process.stderr.write("Can't open PIC output");
      return;
    }

    try {
      this.outputStream = new FileOutputStream(filename);
    }
    catch (_ignored) {
      this.outputStream = null;
      process.stderr.write("Can't open PIC output");
      return;
    }

    this.writeHeader();
  }

  public closeHandle(): void {
    if (this.outputStream !== null) {
      try {
        this.outputStream.close();
      }
      catch (_ignored) {
      }
    }
    this.outputStream = null;
  }

  public override writeRadianceRGB(rgbRadiance: ColorRgb[]): number {
    let result = 0;
    if (this.outputStream !== null) {
      result = DkColor.writeScan(rgbRadiance, this.width, this.outputStream);
    }

    if (result !== 0) {
      return this.width;
    }
    return 0;
  }

  private writeHeader(): void {
    PicOutputHandle.writeFormatted(this.outputStream, "#?RADIANCE\n");
    const compileDate = PicOutputHandle.formatCompileDate(new Date());
    PicOutputHandle.writeFormatted(this.outputStream, "#RPK PicOutputHandler (compiled %s)\n", compileDate);
    PicOutputHandle.writeFormatted(this.outputStream, "FORMAT=32-bit_rle_rgbe\n");
    PicOutputHandle.writeFormatted(this.outputStream, "\n");
    PicOutputHandle.writeFormatted(this.outputStream, "-Y %d +X %d\n", this.height, this.width);
  }
}
