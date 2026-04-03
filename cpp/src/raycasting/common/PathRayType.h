#ifndef __PATH_RAY_TYPE__
#define __PATH_RAY_TYPE__

/**
PathRayType indicates what the ray does further in the path
F.i. it can be reflected, it can enter a material, leave it
or the path can end with this ray
*/
enum PathRayType {
    STARTS,
    ENTERS,
    LEAVES,
    REFLECTS,
    STOPS,
    ENVIRONMENT
};

#endif
