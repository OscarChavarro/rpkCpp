package vsdk.toolkit.raycasting.common;

/**
PathRayType indicates what the ray does further in the path
F.i. it can be reflected, it can enter a material, leave it
or the path can end with this ray
*/
public enum PathRayType {
    STARTS,
    ENTERS,
    LEAVES,
    REFLECTS,
    STOPS,
    ENVIRONMENT
}
