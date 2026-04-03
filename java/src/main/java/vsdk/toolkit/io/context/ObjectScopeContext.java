package vsdk.toolkit.io.context;

import java.util.Arrays;

public class ObjectScopeContext {
    public String[] objectNamesList;
    public int objectMaxName;
    public int objectNames;

    private static final int OBJECT_NAMES_ALLOC_INCREMENT = 16;

    public ObjectScopeContext() {
        objectNamesList = null;
        objectMaxName = 0;
        objectNames = 0;
    }

    public void destroy() {
        clear();
    }

    public int pushName(String name) {
        if (name == null) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        try {
            if (objectNames >= objectMaxName - 1) {
                if (objectMaxName == 0) {
                    objectMaxName = OBJECT_NAMES_ALLOC_INCREMENT;
                    objectNamesList = new String[objectMaxName];
                }
                else {
                    final int previousMaxName = objectMaxName;
                    objectMaxName += OBJECT_NAMES_ALLOC_INCREMENT;
                    objectNamesList = Arrays.copyOf(objectNamesList, objectMaxName);
                    if (objectNamesList == null) {
                        objectMaxName = previousMaxName;
                    }
                }
                if (objectNamesList == null) {
                    objectMaxName = 0;
                    objectNames = 0;
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
            }

            objectNamesList[objectNames] = name;
            objectNames++;
            objectNamesList[objectNames] = null;
            return ParseErrorContext.MGF_OK;
        }
        catch (OutOfMemoryError outOfMemoryError) {
            objectMaxName = 0;
            objectNames = 0;
            objectNamesList = null;
            return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    public int popName() {
        if (objectNames < 1) {
            return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        objectNames--;
        objectNamesList[objectNames] = null;
        return ParseErrorContext.MGF_OK;
    }

    public void clear() {
        if (objectNamesList != null) {
            for (int i = 0; i < objectNames; i++) {
                objectNamesList[i] = null;
            }
        }
        objectNamesList = null;
        objectMaxName = 0;
        objectNames = 0;
    }
}
