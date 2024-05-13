#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "scene/Scene.h"

class RpkApplication {
  private:
    Scene *scene;

  public:
    RpkApplication();
    ~RpkApplication();
    int entryPoint(int argc, char *argv[]);
};

#endif
