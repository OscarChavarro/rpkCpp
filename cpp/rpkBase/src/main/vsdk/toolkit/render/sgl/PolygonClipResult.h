#ifndef __POLYGON_CLIP_RESULT__
#define __POLYGON_CLIP_RESULT__

class PolygonClipResultInfo final {
  public:
    static constexpr int MAXIMUM_SIDES_PER_POLYGON = 10;
};

enum PolygonClipResult {
    POLY_CLIP_OUT = 0,
    POLY_CLIP_PARTIAL = 1,
    POLY_CLIP_IN = 2
};

#endif
