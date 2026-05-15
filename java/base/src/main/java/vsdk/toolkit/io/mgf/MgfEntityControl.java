package vsdk.toolkit.io.mgf;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.dataStructures.LookUpEntity;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.FilePositionContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.wrapper.FileUncompressWrapper;

public class MgfEntityControl {
    private static String standardInputPath() {
        return "/dev/stdin";
    }

    private static boolean skipLines(InputStream inputStream, int lineCount) {
        if (inputStream == null || lineCount < 0) {
            return false;
        }
        try {
            for (int line = 0; line < lineCount; line++) {
                boolean foundEol = false;
                while (true) {
                    final int ch = inputStream.read();
                    if (ch < 0) {
                        return false;
                    }
                    if (ch == '\n') {
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
        catch (Exception e) {
            return false;
        }
    }

    /**
    Default handler for unknown entities
    */
    private static int mgfDefaultHandlerForUnknownEntities(int ac, String[] av, ParseRuntimeContext context) {
        // Just ignore line
        return ParseErrorContext.MGF_OK;
    }

    public static void doError(String errmsg, ParseRuntimeContext context) {
        Logger.error(null, "%s line %d: %s", context.readerContext.fileName, context.readerContext.lineNumber, errmsg);
    }

    public static void doWarning(String errmsg, ParseRuntimeContext context) {
        Logger.warning(null, "%s line %d: %s", context.readerContext.fileName, context.readerContext.lineNumber, errmsg);
    }

    /**
    Get current position in input file
    */
    public static void mgfGetFilePosition(FilePositionContext pos, ParseRuntimeContext context) {
        pos.fileId = context.readerContext.fileContextId;
        pos.lineNumber = context.readerContext.lineNumber;
        pos.offset = -1;
    }

    /**
    Reposition input file pointer
    */
    public static int mgfGoToFilePosition(FilePositionContext pos, ParseRuntimeContext context) {
        if (pos.fileId != context.readerContext.fileContextId) {
            return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
        }
        if (pos.lineNumber == context.readerContext.lineNumber) {
            return ParseErrorContext.MGF_OK;
        }
        if (context.readerContext.inputStream == null) {
            return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
        }
        if ("<stdin>".equals(context.readerContext.fileName) || context.readerContext.isPipe != 0) {
            // Cannot seek on standard input or pipes
            return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
        }
        int[] pipeFlag = new int[] {0};
        InputStream inputStream = FileUncompressWrapper.openInputStreamCompressWrapper(context.readerContext.fileName, pipeFlag);
        if (inputStream == null || pipeFlag[0] != 0) {
            FileUncompressWrapper.closeInputStream(inputStream);
            return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
        }

        if (!MgfEntityControl.skipLines(inputStream, pos.lineNumber)) {
            FileUncompressWrapper.closeInputStream(inputStream);
            return ParseErrorContext.MGF_ERROR_FILE_SEEK_ERROR;
        }

        try {
            context.readerContext.inputStream.close();
        }
        catch (Exception ignored) {
        }
        context.readerContext.inputStream = inputStream;
        context.readerContext.lineNumber = pos.lineNumber;
        return ParseErrorContext.MGF_OK;
    }

    /**
    Get entity number from its name
    */
    public static int mgfEntity(String name, ParseRuntimeContext context) {
        if (context.entityLookUpTable.getCurrentTableSize() == 0) {
            // Initialize hash table
            if (context.entityLookUpTable.lookUpInit(EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES) == 0) {
                return -1;
            }

            for (int i = EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES - 1; i >= 0; i--) {
                String entityName = context.entityNames[i];
                LookUpEntity<String> entity = context.entityLookUpTable.lookUpFind(entityName);
                if (entity != null) {
                    entity.key = entityName;
                }
            }
        }

        LookUpEntity<String> found = context.entityLookUpTable.lookUpFind(name);
        if (found == null || found.key == null) {
            return -1;
        }
        String entityName = found.key;
        for (int i = 0; i < EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES; i++) {
            if (context.entityNames[i] == entityName || context.entityNames[i].equals(entityName)) {
                return i;
            }
        }
        return -1;
    }

    /**
    Pass entity to appropriate handler
    */
    public static int mgfHandle(int entityIndex, int argc, String[] argv, ParseRuntimeContext context) {
        entityIndex = MgfEntityControl.mgfEntity(argv[0], context);
        if (entityIndex < 0) {
            // Unknown entity
            return MgfEntityControl.mgfDefaultHandlerForUnknownEntities(argc, argv, context);
        }
        if (context.readerStackState.supportCallbacks[entityIndex] != null) {
            // Support handler
            int rv = context.readerStackState.supportCallbacks[entityIndex].handle(argc, argv, context);
            if (rv != ParseErrorContext.MGF_OK) {
                return rv;
            }
        }
        return context.readerStackState.handleCallbacks[entityIndex].handle(argc, argv, context); // Assigned handler
    }

    /**
    shaftCullOpen new input file
    */
    public static int mgfOpen(ReaderContext readerContext, String functionCallback, ParseRuntimeContext context) {
        readerContext.fileContextId = ++context.nextFileContextId;
        context.readerStackState.nextFileContextId = context.nextFileContextId;
        readerContext.lineNumber = 0;
        readerContext.isPipe = 0;
        readerContext.inputStream = null;
        if (functionCallback == null) {
            readerContext.fileName = "<stdin>";
            File standardInputFile = new File(standardInputPath());
            if (!standardInputFile.exists() || !standardInputFile.canRead()) {
                return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
            }
            try {
                readerContext.inputStream = new FileInputStream(standardInputPath());
            }
            catch (Exception e) {
                return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
            }
            readerContext.prev = context.readerContext;
            context.readerContext = readerContext;
            context.readerStackState.readerContext = readerContext;
            return ParseErrorContext.MGF_OK;
        }

        // Get name relative to this context
        if (context.readerContext != null) {
            String currentFileName = context.readerContext.fileName;
            int slashIndex = currentFileName.lastIndexOf('/');
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

        int[] pipeFlag = new int[] {0};
        InputStream inputStream = FileUncompressWrapper.openInputStreamCompressWrapper(readerContext.fileName, pipeFlag);
        if (inputStream == null) {
            return ParseErrorContext.MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
        }
        readerContext.isPipe = (char)(pipeFlag[0] != 0 ? 1 : 0);
        readerContext.inputStream = inputStream;

        readerContext.prev = context.readerContext; // Establish new context
        context.readerContext = readerContext;
        context.readerStackState.readerContext = readerContext;
        return ParseErrorContext.MGF_OK;
    }

    /**
    Close input file
    */
    public static void mgfClose(ParseRuntimeContext context) {
        if (context == null || context.readerContext == null) {
            return;
        }
        ReaderContext ctx = context.readerContext;

        context.readerContext = ctx.prev; // Restore enclosing context
        context.readerStackState.readerContext = context.readerContext;
        if (ctx.inputStream != null) {
            // Close file if it's a file
            try {
                ctx.inputStream.close();
            }
            catch (Exception ignored) {
            }
            ctx.inputStream = null;
        }
    }

    public static void mgfLookUpFreeMemory(ParseRuntimeContext context) {
        if (context != null) {
            context.entityLookUpTable.lookUpDone();
        }
    }
}
