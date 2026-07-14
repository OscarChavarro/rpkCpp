#ifndef CANVAS__
#define CANVAS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Canvas {
  private:
    static constexpr int CANVAS_MODE_STACK_SIZE = 5;
    static int modeStackIndex;

  public:
    static void canvasPushMode();
    static void canvasPullMode();
};

#endif
