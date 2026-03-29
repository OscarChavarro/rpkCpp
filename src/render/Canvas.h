#ifndef __CANVAS__
#define __CANVAS__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

class Canvas {
  public:
    static void canvasPushMode();
    static void canvasPullMode();
};

#endif
