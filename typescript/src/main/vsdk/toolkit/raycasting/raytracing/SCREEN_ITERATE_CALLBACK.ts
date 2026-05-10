import { ColorRgb } from "../../common/color/ColorRgb";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";

export interface SCREEN_ITERATE_CALLBACK {
  call(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    x: number,
    y: number,
    data: unknown
  ): ColorRgb;
}
