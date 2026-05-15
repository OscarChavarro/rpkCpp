package vsdk.toolkit.raycasting.raytracing;

public final class SampleConnectionFlags {
    public static final int CONNECT_EL = 0x01; // Compute pdf(E->L) and bsdf(EP -> E -> L)
    public static final int CONNECT_LE = 0x02; // Compute pdf(L->E) and bsdf(LP -> L -> E)
    public static final int FILL_OTHER_PDF = 0x10; // If CONNECT_EL or CONNECT_LE then also compute the opposite PDF.

    private SampleConnectionFlags() {
    }
}
