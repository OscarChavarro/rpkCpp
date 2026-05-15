package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.*;

public final class ElementFlags {
    // If set, indicates that the element is a cluster element. If not set, the element is a surface element
    public static final int IS_CLUSTER_MASK = 0x01;
    // If the element is or contains surfaces emitting light spontaneously
    public static final int IS_LIGHT_SOURCE_MASK = 0x02;
    // Set when all interactions have been created for a toplevel element
    public static final int INTERACTIONS_CREATED_MASK = 0x04;

    private ElementFlags() {
    }
}
