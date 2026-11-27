package vsdk.toolkit.galerkin;

import vsdk.toolkit.render.sgl.SglVisibilityBuffer;

/**
Borrows surface elements for the current solve. A null sample is background.
Java adaptation: the C++ port's alias of SglVisibilityBuffer<const GalerkinElement *>.
*/
public final class GalerkinElementVisibilityBuffer extends SglVisibilityBuffer<GalerkinElement> {
    public GalerkinElementVisibilityBuffer(int width, int height, GalerkinElement backgroundValue) {
        super(width, height, backgroundValue);
    }
}
