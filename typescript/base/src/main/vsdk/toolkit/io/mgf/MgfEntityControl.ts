import { File } from "../../../../java/io/File";
import { FileInputStream } from "../../../../java/io/FileInputStream";
import { InputStream } from "../../../../java/io/InputStream";
import { Logger } from "../../common/logging/Logger";
import { LookUpEntity } from "../../common/dataStructures/LookUpEntity";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { FilePositionContext } from "../context/FilePositionContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ReaderContext } from "../context/ReaderContext";
import { FileUncompressWrapper } from "../wrapper/FileUncompressWrapper";

export class MgfEntityControl {
  private constructor() {
  }

  private static standardInputPath(): string {
    return "/dev/stdin";
  }

  private static skipLines(inputStream: InputStream | null, lineCount: number): boolean {
    if (inputStream === null || lineCount < 0) {
      return false;
    }
    try {
      for (let line = 0; line < lineCount; line++) {
        let foundEol = false;
        while (true) {
          const ch = inputStream.read();
          if (ch < 0) {
            return false;
          }
          if (ch === "\n".charCodeAt(0)) {
            foundEol = true;
            break;
          }
        }
        if (!foundEol) {
          return false;
        }
      }
      return true;
    }
    catch (_e) {
      return false;
    }
  }

  /**
  Default handler for unknown entities
  */
  private static mgfDefaultHandlerForUnknownEntities(ac: number, av: string[], context: ParseRuntimeContext): number {
    void ac;
    void av;
    void context;
    // Just ignore line
    return ParseErrorContext.MGF_OK;
  }

  public static doError(errmsg: string, context: ParseRuntimeContext): void {
    const readerContext = context.readerContext;
    const fileName = readerContext?.fileName ?? "<unknown>";
    const lineNumber = readerContext?.lineNumber ?? 0;
    Logger.error(null, "%s line %d: %s", fileName, lineNumber, errmsg);
  }

  public static doWarning(errmsg: string, context: ParseRuntimeContext): void {
    const readerContext = context.readerContext;
    const fileName = readerContext?.fileName ?? "<unknown>";
    const lineNumber = readerContext?.lineNumber ?? 0;
    Logger.warning(null, "%s line %d: %s", fileName, lineNumber, errmsg);
  }

  /**
  Get current position in input file
  */
  public static mgfGetFilePosition(pos: FilePositionContext, context: ParseRuntimeContext): void {
    const readerContext = context.readerContext;
    pos.fileId = readerContext?.fileContextId ?? 0;
    pos.lineNumber = readerContext?.lineNumber ?? 0;
    pos.offset = -1;
  }

  /**
  Reposition input file pointer
  */
  public static mgfGoToFilePosition(pos: FilePositionContext, context: ParseRuntimeContext): number {
    const readerContext = context.readerContext;
    if (readerContext === null) {
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }
    if (pos.fileId !== readerContext.fileContextId) {
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }
    if (pos.lineNumber === readerContext.lineNumber) {
      return ParseErrorContext.MGF_OK;
    }
    if (readerContext.inputStream === null) {
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }
    if (readerContext.fileName === "<stdin>" || readerContext.isPipe !== 0) {
      // Cannot seek on standard input or pipes
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }
    const pipeFlag = [0];
    const inputStream = FileUncompressWrapper.openInputStreamCompressWrapper(readerContext.fileName, pipeFlag);
    if (inputStream === null || pipeFlag[0] !== 0) {
      FileUncompressWrapper.closeInputStream(inputStream);
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }

    if (!MgfEntityControl.skipLines(inputStream, pos.lineNumber)) {
      FileUncompressWrapper.closeInputStream(inputStream);
      return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
    }

    try {
      readerContext.inputStream.close();
    }
    catch (_ignored) {
    }
    readerContext.inputStream = inputStream;
    readerContext.lineNumber = pos.lineNumber;
    return ParseErrorContext.MGF_OK;
  }

  /**
  Get entity number from its name
  */
  public static mgfEntity(name: string, context: ParseRuntimeContext): number {
    if (context.entityLookUpTable.getCurrentTableSize() === 0) {
      // Initialize hash table
      if (context.entityLookUpTable.lookUpInit(EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES) === 0) {
        return -1;
      }

      for (let i = EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES - 1; i >= 0; i--) {
        const entityName = context.entityNames[i]!;
        const entity = context.entityLookUpTable.lookUpFind(entityName);
        if (entity !== null) {
          entity.key = entityName;
        }
      }
    }

    const found = context.entityLookUpTable.lookUpFind(name) as LookUpEntity<string> | null;
    if (found === null || found.key === null) {
      return -1;
    }
    const entityName = found.key;
    for (let i = 0; i < EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES; i++) {
      if (context.entityNames[i] === entityName) {
        return i;
      }
    }
    return -1;
  }

  /**
  Pass entity to appropriate handler
  */
  public static mgfHandle(entityIndex: number, argc: number, argv: string[], context: ParseRuntimeContext): number {
    entityIndex = MgfEntityControl.mgfEntity(argv[0]!, context);
    if (entityIndex < 0) {
      // Unknown entity
      return MgfEntityControl.mgfDefaultHandlerForUnknownEntities(argc, argv, context);
    }
    const supportHandler = context.readerStackState.supportCallbacks[entityIndex];
    if (supportHandler !== null) {
      // Support handler
      const rv = supportHandler!.handle(argc, argv, context);
      if (rv !== ParseErrorContext.MGF_OK) {
        return rv;
      }
    }
    const handler = context.readerStackState.handleCallbacks[entityIndex];
    if (handler === null) {
      return ParseErrorContext.MGF_OK;
    }
    return handler!.handle(argc, argv, context); // Assigned handler
  }

  /**
  Open new input file
  */
  public static mgfOpen(readerContext: ReaderContext, functionCallback: string | null, context: ParseRuntimeContext): number {
    readerContext.fileContextId = ++context.nextFileContextId;
    context.readerStackState.nextFileContextId = context.nextFileContextId;
    readerContext.lineNumber = 0;
    readerContext.isPipe = 0;
    readerContext.inputStream = null;
    if (functionCallback === null) {
      readerContext.fileName = "<stdin>";
      const standardInputFile = new File(MgfEntityControl.standardInputPath());
      const isReadable = standardInputFile.exists() && standardInputFile.canRead();
      standardInputFile.dispose();
      if (!isReadable) {
        return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
      }
      readerContext.inputStream = new FileInputStream(MgfEntityControl.standardInputPath());
      if (readerContext.inputStream === null) {
        return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
      }
      readerContext.prev = context.readerContext;
      context.readerContext = readerContext;
      context.readerStackState.readerContext = readerContext;
      return ParseErrorContext.MGF_OK;
    }

    // Get name relative to this context
    if (context.readerContext !== null) {
      const currentFileName = context.readerContext.fileName;
      const slashIndex = currentFileName.lastIndexOf("/");
      if (slashIndex >= 0) {
        readerContext.fileName = currentFileName.substring(0, slashIndex + 1) + functionCallback;
      }
      else {
        readerContext.fileName = functionCallback;
      }
    }
    else {
      readerContext.fileName = functionCallback;
    }

    const pipeFlag = [0];
    const inputStream = FileUncompressWrapper.openInputStreamCompressWrapper(readerContext.fileName, pipeFlag);
    if (inputStream === null) {
      return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }
    readerContext.isPipe = pipeFlag[0] !== 0 ? 1 : 0;
    readerContext.inputStream = inputStream;

    readerContext.prev = context.readerContext; // Establish new context
    context.readerContext = readerContext;
    context.readerStackState.readerContext = readerContext;
    return ParseErrorContext.MGF_OK;
  }

  /**
  Close input file
  */
  public static mgfClose(context: ParseRuntimeContext): void {
    if (context === null || context.readerContext === null) {
      return;
    }
    const ctx = context.readerContext;

    context.readerContext = ctx.prev; // Restore enclosing context
    context.readerStackState.readerContext = context.readerContext;
    if (ctx.inputStream !== null) {
      // Close file if it's a file
      try {
        ctx.inputStream.close();
      }
      catch (_ignored) {
      }
      ctx.inputStream = null;
    }
  }

  public static mgfLookUpFreeMemory(context: ParseRuntimeContext): void {
    if (context !== null) {
      context.entityLookUpTable.lookUpDone();
    }
  }
}
