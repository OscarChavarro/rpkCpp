package vsdk.toolkit.io.context;

public abstract class EntityDispatchContext {
    public abstract int handle(int argc, String[] argv, ParseContext context);

    public abstract HandlerRoleContext type();
}
