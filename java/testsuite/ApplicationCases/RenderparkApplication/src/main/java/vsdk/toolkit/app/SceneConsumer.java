package vsdk.toolkit.app;

import vsdk.toolkit.scene.Scene;

/**
Receives the scene RpkApplication.entryPoint builds from a command line,
in place of the rendering. Test programs (such as the ray corpus replay of
testsuite/ApplicationCases/RayCorpusReplay) use it to obtain exactly the
scene a rendering run of the same command line would use.
*/
public interface SceneConsumer {
    /**
    @return the exit status of the program
    */
    int consume(Scene scene);
}
