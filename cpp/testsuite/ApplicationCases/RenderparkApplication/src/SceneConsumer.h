#ifndef SCENE_CONSUMER__
#define SCENE_CONSUMER__

#include "vsdk/toolkit/scene/Scene.h"

/**
Receives the scene RpkApplication::entryPoint builds from a command line,
in place of the rendering. Test programs (such as the ray corpus replay of
cpp/testsuite/ApplicationCases/RayCorpusReplay) use it to obtain exactly the
scene a rendering run of the same command line would use.
*/
class SceneConsumer {
  public:
    virtual ~SceneConsumer() {}

    /**
    @return the exit status of the program
    */
    virtual int consume(const Scene &scene) = 0;
};

#endif
