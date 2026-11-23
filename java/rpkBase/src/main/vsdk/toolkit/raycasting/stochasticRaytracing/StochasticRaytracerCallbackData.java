package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.scene.RadianceMethod;

public class StochasticRaytracerCallbackData {
    public final StochasticRaytracingConfiguration config;
    public final RadianceMethod radianceMethod;
    public final RenderOptions renderOptions;

    public StochasticRaytracerCallbackData(StochasticRaytracingConfiguration config,
                                           RadianceMethod radianceMethod, RenderOptions renderOptions) {
        this.config = config;
        this.radianceMethod = radianceMethod;
        this.renderOptions = renderOptions;
    }
}
