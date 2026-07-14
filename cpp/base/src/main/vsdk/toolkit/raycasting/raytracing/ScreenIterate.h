/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

#ifndef SCREEN_ITERATE__
#define SCREEN_ITERATE__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterateState.h"

using SCREEN_ITERATE_CALLBACK = ColorRgbMutable (*)(Camera *, VoxelGrid *, Background *, int, int, void *);

class ScreenIterate {
  public:
    static void
    sequential(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SCREEN_ITERATE_CALLBACK callback,
        void *data,
        const ToneMappingContext &toneMapOptions);

    static void
    progressive(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SCREEN_ITERATE_CALLBACK callback,
        void *data,
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
        ColorRgbMutable col,
        ColorRgbMutable *rgb);
};

#endif
