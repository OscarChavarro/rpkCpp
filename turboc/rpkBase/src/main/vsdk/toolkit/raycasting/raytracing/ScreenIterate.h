/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

#ifndef __SCREEN_ITERATE__
#define __SCREEN_ITERATE__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterateState.h"

class ScreenPixelCallback {
  public:
    virtual ~ScreenPixelCallback() {}
    virtual ColorRgb call(Camera *camera, const SceneQuery *query, SceneQueryWorkspace *workspace,
                          Background *background, int x, int y) const = 0;
};

class ScreenIterate {
  public:
    static void
    sequential(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        const ScreenPixelCallback &callback,
        const ToneMappingContext &toneMapOptions);

    static void
    progressive(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        const ScreenPixelCallback &callback,
        const ToneMappingContext &toneMapOptions);

  private:
    static ScreenIterateState state;

    static unsigned char wakeUpRender();
    static void init();
    static void finish();
    static void updateCpuSecs();
    static void
    fillRect(
        const Camera *camera,
        int x0,
        int y0,
        int x1,
        int y1,
        ColorRgb col,
        ColorRgb *rgb);
};

#endif
