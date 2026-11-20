import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import type { VoxelGridWorkspace } from "../../scene/VoxelGridWorkspace";

export type SCREEN_ITERATE_CALLBACK = (
  camera: Camera,
  sceneVoxelGrid: VoxelGrid,
  sceneVoxelGridWorkspace: VoxelGridWorkspace,
  sceneBackground: Background | null,
  x: number,
  y: number,
  data: unknown
) => ColorRgbMutable;
