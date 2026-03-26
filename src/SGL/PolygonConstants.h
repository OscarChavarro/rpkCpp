#ifndef RPK_SGL_POLYGON_CONSTANTS_H
#define RPK_SGL_POLYGON_CONSTANTS_H

constexpr int MAXIMUM_SIDES_PER_POLYGON = 10;

enum PolygonClipResult {
    POLY_CLIP_OUT = 0,
    POLY_CLIP_PARTIAL = 1,
    POLY_CLIP_IN = 2
};

#endif
