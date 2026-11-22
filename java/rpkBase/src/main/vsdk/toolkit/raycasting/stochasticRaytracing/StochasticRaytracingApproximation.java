package vsdk.toolkit.raycasting.stochasticRaytracing;

public enum StochasticRaytracingApproximation {
    CONSTANT, // 1
    LINEAR, // 1, u, v
    BI_LINEAR, // 1, u, v, uv
    QUADRATIC, // 1, u, v, uv, u2, v2
    CUBIC // 1, u, v, uv, u2, v2, u3, u2v, uv2, v3
}
