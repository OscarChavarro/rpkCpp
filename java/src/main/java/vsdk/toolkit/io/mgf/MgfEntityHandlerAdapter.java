package vsdk.toolkit.io.mgf;

import vsdk.toolkit.io.context.EntityDispatchContext;
import vsdk.toolkit.io.context.HandlerRoleContext;
import vsdk.toolkit.io.context.ParseContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;

public final class MgfEntityHandlerAdapter extends EntityDispatchContext {
    @FunctionalInterface
    public interface HandlerFunction {
        int handle(int argc, String[] argv, ParseRuntimeContext context);
    }

    private final HandlerRoleContext handlerType;
    private final HandlerFunction handlerFunction;

    public MgfEntityHandlerAdapter(HandlerRoleContext handlerType, HandlerFunction handlerFunction) {
        this.handlerType = handlerType;
        this.handlerFunction = handlerFunction;
    }

    @Override
    public int handle(int argc, String[] argv, ParseContext context) {
        if (handlerFunction == null) {
            return ParseErrorContext.MGF_OK;
        }
        return handlerFunction.handle(argc, argv, (ParseRuntimeContext)context);
    }

    @Override
    public HandlerRoleContext type() {
        return handlerType;
    }
}
