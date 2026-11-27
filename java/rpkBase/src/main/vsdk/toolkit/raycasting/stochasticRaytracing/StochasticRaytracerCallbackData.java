package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.scene.RadianceMethod;

public class StochasticRaytracerCallbackData {
    public final StochasticRaytracingConfiguration config;
    public final RadianceMethod radianceMethod;
    public final RendererConfiguration renderOptions;

    public StochasticRaytracerCallbackData(StochasticRaytracingConfiguration config,
                                           RadianceMethod radianceMethod, RendererConfiguration renderOptions) {
        this.config = config;
        this.radianceMethod = radianceMethod;
        this.renderOptions = renderOptions;
    }
}
