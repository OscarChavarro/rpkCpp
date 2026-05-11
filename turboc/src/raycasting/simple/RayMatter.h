#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RAY_MATTER__
#define __RAY_MATTER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/common/PixelFilter.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/simple/RayMatterFilterType.h"

#define RAY_MATTER_NAME "Ray Matting"

class RayMatter: public RayTracer{ private:
    static RayMatter *rayMatter;
    ScreenBuffer *screenBuffer;
    PixelFilter *pixelFilter;
    bool doDeleteScreen;
    RayMatterState &rayMatterState;

  public:
    explicit RayMatter( ScreenBuffer *screen, const Camera *camera, RayMatterState &inRayMatterState, ToneMappingContext *toneMapOptions = NULL);
    ~RayMatter();

    void createFilter();
    void doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid);
    void display();
    void save(ImageOutputHandle *ip);

    void defaults();
    const char *getName() const;
    void initialize(const ArrayList<Patch *> *lightPatches) const;

    void
    execute( ImageOutputHandle *ip, Scene *scene, RadianceMethod *radianceMethod, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions) const;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const;
    void terminate() const;
};

#endif
#endif
