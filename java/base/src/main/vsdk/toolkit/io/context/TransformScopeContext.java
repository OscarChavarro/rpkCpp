package vsdk.toolkit.io.context;

import java.util.Arrays;

public class TransformScopeContext {
    public TransformStackContext transformContext;
    public String[] argumentList;
    public int argumentCount;
    public String iterateArgument;

    public TransformScopeContext() {
        transformContext = null;
        argumentList = null;
        argumentCount = 0;
        iterateArgument = "-i";
    }

    public void destroy() {
        clearArguments();
    }

    public void clearArguments() {
        argumentList = null;
        argumentCount = 0;
    }

    public int argumentCountFor(TransformStackContext context) {
        return context == null ? 0 : context.xac;
    }

    public int argumentStartIndexFor(TransformStackContext context) {
        return argumentCount - argumentCountFor(context);
    }

    public String[] argumentVectorFor(TransformStackContext context) {
        if (argumentList == null) {
            return null;
        }
        int startIndex = argumentStartIndexFor(context);
        if (startIndex < 0 || startIndex >= argumentList.length) {
            return null;
        }
        return Arrays.copyOfRange(argumentList, startIndex, argumentList.length);
    }

    public boolean compactTo(TransformStackContext context) {
        final int contextArgumentCount = argumentCountFor(context);
        String[] newArgumentList = null;

        if (contextArgumentCount > 0) {
            newArgumentList = new String[contextArgumentCount + 1];

            final int sourceStartIndex = argumentCount - contextArgumentCount;
            for (int i = 0; i < contextArgumentCount; i++) {
                newArgumentList[i] = argumentList[sourceStartIndex + i];
            }
            newArgumentList[contextArgumentCount] = null;
        }

        argumentList = newArgumentList;
        argumentCount = contextArgumentCount;
        return true;
    }

    public void freeTransformContext(TransformStackContext context) {
        if (context == null) {
            return;
        }
        if (context.ownedArgumentCopies != null) {
            for (int i = 0; i < context.ownedArgumentCount; i++) {
                context.ownedArgumentCopies[i] = null;
            }
            context.ownedArgumentCopies = null;
        }
        if (context.transformationArray != null) {
            context.transformationArray = null;
        }
    }
}
