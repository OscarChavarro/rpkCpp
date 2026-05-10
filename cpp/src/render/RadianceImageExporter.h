#ifndef __RADIANCE_IMAGE_EXPORTER__
#define __RADIANCE_IMAGE_EXPORTER__

#include "java/io/OutputStream.h"
#include "common/color/ColorRgb.h"
#include "material/RendererConfiguration.h"
#include "scene/Camera.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "environment/geometry/elements/Patch.h"
#include "render/ScreenBuffer.h"
#include "tonemap/ToneMappingContext.h"

class RadianceImageExporter final {
  public:
    static void exportImage(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);

  private:
    static void clipUv(int numberOfVertices, double *u, double *v);
    static ColorRgb getRadianceAtPixel(
        const ScreenBuffer *screenBuffer,
        Camera *camera,
        int x,
        int y,
        Patch *patch,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions);
};

#endif
