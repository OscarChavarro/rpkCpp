#ifndef RADIANCE_IMAGE_EXPORTER__
#define RADIANCE_IMAGE_EXPORTER__

#include "vsdk/toolkit/java/io/OutputStream.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

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
    static ColorRgbMutable getRadianceAtPixel(
        const ScreenBuffer *screenBuffer,
        Camera *camera,
        int x,
        int y,
        Patch *patch,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions);
};

#endif
