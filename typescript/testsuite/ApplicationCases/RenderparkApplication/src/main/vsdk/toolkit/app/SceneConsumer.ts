import type { Scene } from "vitral/dist/vsdk/toolkit/scene/Scene";

/**
Receives the scene RpkApplication.entryPoint builds from a command line,
in place of the rendering. Test programs use it to obtain exactly the scene
a rendering run of the same command line would use.
*/
export abstract class SceneConsumer {
  /**
  @return the exit status of the program
  */
  public abstract consume(scene: Scene): number;
}
