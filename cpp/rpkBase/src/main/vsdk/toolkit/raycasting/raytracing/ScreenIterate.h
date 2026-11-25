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
#include "vsdk/toolkit/scene/Camera.h"

class SceneQuery;
class SceneQueryWorkspace;

template<typename T>
using ScreenIterateCallback = ColorRgb (*)(Camera *, const SceneQuery *, SceneQueryWorkspace *, Background *, int, int, T *);

class ScreenIterate {
  public:
    template<typename T>
    static void
    sequential(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        ScreenIterateCallback<T> callback,
        T *data,
        const ToneMappingContext &toneMapOptions);

    template<typename T>
    static void
    progressive(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        ScreenIterateCallback<T> callback,
        T *data,
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

#include "vsdk/toolkit/raycasting/raytracing/ScreenIterate.txx"

#endif
