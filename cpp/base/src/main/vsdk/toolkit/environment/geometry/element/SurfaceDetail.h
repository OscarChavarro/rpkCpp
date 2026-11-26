#ifndef __SURFACE_DETAIL__
#define __SURFACE_DETAIL__

// Shared query vocabulary; renderers choose which details they request.
class SurfaceDetail {
public:
    enum {
        NONE = 0,
        POINT = 1 << 0,
        NORMAL = 1 << 1,
        UV = 1 << 2,
        TANGENT = 1 << 3,
        ALL = POINT | NORMAL | UV | TANGENT
    };
};

#endif
