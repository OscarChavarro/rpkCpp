package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.skin.Patch;

public class LightSourceTable {
    public Patch patch;
    public double flux;

    public LightSourceTable() {
        patch = null;
        flux = 0.0;
    }

    public LightSourceTable(Patch patch, double flux) {
        this.patch = patch;
        this.flux = flux;
    }
}
