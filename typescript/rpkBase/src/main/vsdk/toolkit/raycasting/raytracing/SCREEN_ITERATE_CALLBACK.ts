import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";

export type SCREEN_ITERATE_CALLBACK<T> = (
  camera: Camera,
  sceneQuery: SceneQuery,
  sceneSceneQueryWorkspace: SceneQueryWorkspace,
  sceneBackground: Background | null,
  x: number,
  y: number,
  data: T
) => ColorRgbMutable;
