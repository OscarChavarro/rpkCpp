import { File } from "../../../../java/io/File";
import { FileInputStream } from "../../../../java/io/FileInputStream";
import { FileOutputStream } from "../../../../java/io/FileOutputStream";
import { InputStream } from "../../../../java/io/InputStream";
import { OutputStream } from "../../../../java/io/OutputStream";
import { Logger } from "../../common/logging/Logger";
import { PipeInputStream } from "./PipeInputStream";
import { PipeOutputStream } from "./PipeOutputStream";
import { StreamOpenMode } from "./StreamOpenMode";

type CommandHolder = {
  value: string;
};

export class FileUncompressWrapper {
  public static openInputStreamCompressWrapper(fileName: string | null, isPipe: number[] | null): InputStream | null {
    if (isPipe !== null && isPipe.length > 0) {
      isPipe[0] = 0;
    }
    if (FileUncompressWrapper.isInvalidFileName(fileName)) {
      return null;
    }
    const safeFileName = fileName as string;

    const commandLength = safeFileName.length + 20;
    const command: CommandHolder = { value: "" };
    const pipeFlag = FileUncompressWrapper.buildPipeCommand(safeFileName, StreamOpenMode.READ, command, commandLength);

    let stream: InputStream | null = null;
    if (pipeFlag) {
      stream = FileUncompressWrapper.openPipeInputStream(command.value);
    }
    else {
      const file = new File(safeFileName);
      const canOpen = file.exists() && file.canRead() && file.isFile();
      file.dispose();
      if (canOpen) {
        stream = new FileInputStream(safeFileName);
      }
    }

    if (stream === null) {
      Logger.error(null, "Can't open file '%s' for %s", safeFileName, FileUncompressWrapper.modeToLogAction(StreamOpenMode.READ));
      if (isPipe !== null && isPipe.length > 0) {
        isPipe[0] = 0;
      }
      return null;
    }

    if (isPipe !== null && isPipe.length > 0) {
      isPipe[0] = pipeFlag ? 1 : 0;
    }
    return stream;
  }

  public static openOutputStreamCompressWrapper(fileName: string | null, isPipe: number[] | null): OutputStream | null {
    if (isPipe !== null && isPipe.length > 0) {
      isPipe[0] = 0;
    }
    if (FileUncompressWrapper.isInvalidFileName(fileName)) {
      return null;
    }
    const safeFileName = fileName as string;

    const commandLength = safeFileName.length + 20;
    const command: CommandHolder = { value: "" };
    const pipeFlag = FileUncompressWrapper.buildPipeCommand(safeFileName, StreamOpenMode.WRITE, command, commandLength);

    let stream: OutputStream | null = null;
    if (pipeFlag) {
      stream = FileUncompressWrapper.openPipeOutputStream(command.value);
    }
    else {
      const file = new File(safeFileName);
      const isDirectory = file.isDirectory();
      file.dispose();
      if (!isDirectory) {
        stream = new FileOutputStream(safeFileName);
      }
    }

    if (stream === null) {
      Logger.error(null, "Can't open file '%s' for %s", safeFileName, FileUncompressWrapper.modeToLogAction(StreamOpenMode.WRITE));
      if (isPipe !== null && isPipe.length > 0) {
        isPipe[0] = 0;
      }
      return null;
    }

    if (isPipe !== null && isPipe.length > 0) {
      isPipe[0] = pipeFlag ? 1 : 0;
    }
    return stream;
  }

  public static closeInputStream(stream: InputStream | null): void {
    if (stream === null) {
      return;
    }
    try {
      stream.close();
    }
    catch (_error) {
    }
  }

  public static closeOutputStream(stream: OutputStream | null): void {
    if (stream === null) {
      return;
    }
    try {
      stream.close();
    }
    catch (_error) {
    }
  }

  private static modeToLogAction(mode: StreamOpenMode): string {
    return mode === StreamOpenMode.READ ? "reading" : "writing";
  }

  private static isInvalidFileName(fileName: string | null): boolean {
    if (fileName === null || fileName.length === 0 || fileName.endsWith("/")) {
      return true;
    }
    return false;
  }

  private static buildPipeCommand(
    fileName: string | null,
    openMode: StreamOpenMode,
    command: CommandHolder | null,
    commandLength: number
  ): boolean {
    if (fileName === null || command === null || commandLength <= 0) {
      return false;
    }

    command.value = "";

    if (fileName.charAt(0) === "|") {
      command.value = fileName.substring(1);
      return true;
    }

    const dot = fileName.lastIndexOf(".");
    const ext = dot >= 0 ? fileName.substring(dot) : null;

    if (ext === ".gz") {
      if (openMode === StreamOpenMode.READ) {
        command.value = `gunzip < ${fileName}`;
      }
      else {
        command.value = `gzip > ${fileName}`;
      }
    }
    else if (ext === ".Z") {
      if (openMode === StreamOpenMode.READ) {
        command.value = `uncompress < ${fileName}`;
      }
      else {
        command.value = `compress > ${fileName}`;
      }
    }
    else if (ext === ".bz") {
      if (openMode === StreamOpenMode.READ) {
        command.value = `bunzip < ${fileName}`;
      }
      else {
        command.value = `bzip > ${fileName}`;
      }
    }
    else if (ext === ".bz2") {
      if (openMode === StreamOpenMode.READ) {
        command.value = `bunzip2 < ${fileName}`;
      }
      else {
        command.value = `bzip2 > ${fileName}`;
      }
    }
    else {
      return false;
    }
    return true;
  }

  private static openPipeInputStream(command: string): InputStream | null {
    const pipeStream = new PipeInputStream(command);
    if (!pipeStream.isOpen()) {
      return null;
    }
    return pipeStream;
  }

  private static openPipeOutputStream(command: string): OutputStream | null {
    const pipeStream = new PipeOutputStream(command);
    if (!pipeStream.isOpen()) {
      return null;
    }
    return pipeStream;
  }
}
