package vsdk.toolkit.galerkin;

// Element render modes, additive
public enum GalerkinElementRenderMode {
    OUTLINE(0x01),
    FLAT(0x02),
    GOURAUD(0x04),
    NOT_A_REGULAR_SUB_ELEMENT(0x08);

    public final int value;

    GalerkinElementRenderMode(int value) {
        this.value = value;
    }
}
