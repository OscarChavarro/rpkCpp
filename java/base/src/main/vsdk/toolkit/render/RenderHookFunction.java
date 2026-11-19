package vsdk.toolkit.render;

@FunctionalInterface
public interface RenderHookFunction {
    void apply(Object data);
}
