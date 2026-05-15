#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CANVAS__
#define __CANVAS__

#include "java/util/ArrayList.h"
#include "environment/geometry/elements/Patch.h"

class Canvas {
  private:
    #define CANVAS_MODE_STACK_SIZE 5
    static int modeStackIndex;

  public:
    static void canvasPushMode();
    static void canvasPullMode();
};

#endif
