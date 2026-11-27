package vsdk.toolkit.io.context;

import java.util.ArrayList;

public class ObjectScopeContext {
    private final ArrayList<String> objectNamesList;

    public ObjectScopeContext() {
        objectNamesList = new ArrayList<>();
    }

    public void destroy() {
        clear();
    }

    public int pushName(String name) {
        if ( name == null ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        try {
            objectNamesList.add(name);
        } catch ( OutOfMemoryError outOfMemoryError ) {
            return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        return ParseErrorContext.MGF_OK;
    }

    public int popName() {
        final int count = objectNamesList.size();
        if ( count < 1 ) {
            return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        objectNamesList.remove(count - 1);
        return ParseErrorContext.MGF_OK;
    }

    public void clear() {
        objectNamesList.clear();
    }
}
