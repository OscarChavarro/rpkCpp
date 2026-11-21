package vsdk.toolkit.galerkin;

/**
When the Galerkin solver culls the candidate occluders of an interaction with
a shaft (RenderPark's SHAFTCULLMODE). With NEVER_DO_SHAFT_CULLING every shadow
ray that the shadow cache does not answer goes to the full-scene index.
*/
public enum GalerkinShaftCullMode {
    ALWAYS_DO_SHAFT_CULLING,
    DO_SHAFT_CULLING_FOR_REFINEMENT,
    NEVER_DO_SHAFT_CULLING
}
