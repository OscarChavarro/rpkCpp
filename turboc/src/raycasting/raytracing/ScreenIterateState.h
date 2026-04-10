#ifndef __SCREEN_ITERATE_STATE__
#define __SCREEN_ITERATE_STATE__

#include "common/VSDK.h"

class ScreenIterateState {
  public:
    long lastTime;
    unsigned char wakeUp;

    ScreenIterateState();
};

#endif
