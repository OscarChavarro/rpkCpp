import { ColorRgb } from "../../common/color/ColorRgb";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";

export type SCREEN_ITERATE_CALLBACK = (
  camera: Camera,
  sceneVoxelGrid: VoxelGrid,
  sceneBackground: Background | null,
  x: number,
  y: number,
  data: unknown
) => ColorRgb;
