package vsdk.toolkit.io.context;

import vsdk.toolkit.common.dataStructures.LookUpBehaviors;
import vsdk.toolkit.common.dataStructures.LookUpTable;

public class ReaderDispatchContext {
    private static final int TOTAL_MGF_HANDLER_TYPES = HandlerRoleContext.HANDLE_OBJECT.ordinal() + 1;

    public String[] entityNames;
    public String[] errorCodeMessages;
    public LookUpTable<String> entityLookUpTable;
    public int nextFileContextId;
    public ReaderContext readerContext;
    public EntityDispatchContext[] handleCallbacks;
    public EntityDispatchContext[] supportCallbacks;
    public EntityDispatchContext[] handlerByType;

    public static int handlerTypeCount() {
        return TOTAL_MGF_HANDLER_TYPES;
    }

    public ReaderDispatchContext() {
        entityNames = new String[EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES];
        errorCodeMessages = new String[ParseErrorContext.MGF_NUMBER_OF_ERRORS];
        entityLookUpTable = new LookUpTable<>(LookUpBehaviors.NON_OWNING);
        nextFileContextId = 0;
        readerContext = null;
        handleCallbacks = new EntityDispatchContext[EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES];
        supportCallbacks = new EntityDispatchContext[EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES];
        handlerByType = new EntityDispatchContext[TOTAL_MGF_HANDLER_TYPES];

        entityNames[0] = "#";
        entityNames[1] = "c";
        entityNames[2] = "cct";
        entityNames[3] = "cone";
        entityNames[4] = "cmix";
        entityNames[5] = "cspec";
        entityNames[6] = "cxy";
        entityNames[7] = "cyl";
        entityNames[8] = "ed";
        entityNames[9] = "f";
        entityNames[10] = "i";
        entityNames[11] = "ies";
        entityNames[12] = "ir";
        entityNames[13] = "m";
        entityNames[14] = "n";
        entityNames[15] = "o";
        entityNames[16] = "p";
        entityNames[17] = "prism";
        entityNames[18] = "rd";
        entityNames[19] = "ring";
        entityNames[20] = "rs";
        entityNames[21] = "sides";
        entityNames[22] = "sph";
        entityNames[23] = "td";
        entityNames[24] = "torus";
        entityNames[25] = "ts";
        entityNames[26] = "v";
        entityNames[27] = "xf";
        entityNames[28] = "fh";

        errorCodeMessages[0] = "No error";
        errorCodeMessages[1] = "Unknown entity";
        errorCodeMessages[2] = "Wrong number of arguments";
        errorCodeMessages[3] = "Wrong argument type";
        errorCodeMessages[4] = "Illegal argument value";
        errorCodeMessages[5] = "Undefined reference";
        errorCodeMessages[6] = "Cannot open input file";
        errorCodeMessages[7] = "Error in included file";
        errorCodeMessages[8] = "Out of memory";
        errorCodeMessages[9] = "Seek failure";
        errorCodeMessages[10] = "Illegal material specification";
        errorCodeMessages[11] = "Input line too long";
        errorCodeMessages[12] = "Unmatched context close";
    }

    public void destroy() {
        for (int i = 0; i < ReaderDispatchContext.handlerTypeCount(); i++) {
            if (handlerByType[i] != null) {
                handlerByType[i] = null;
            }
        }
    }
}
