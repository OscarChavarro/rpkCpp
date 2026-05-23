import { ColorRgb } from "../../common/color/ColorRgb";
import { Statistics } from "../../common/statistics/Statistics";
import { SoftIds } from "../../render/SoftIds";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { ToneMap } from "../../tonemap/ToneMap";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { SCREEN_ITERATE_CALLBACK } from "./SCREEN_ITERATE_CALLBACK";
import { ScreenIterateState } from "./ScreenIterateState";

export class ScreenIterate {
  private static state = new ScreenIterateState();

  private static wakeUpRender(): number {
    return 1 << 1;
  }

  private static updateCpuSecs(): void {
    const now = process.hrtime.bigint();
    Statistics.instance().rayTracer.totalTime += Number(now - ScreenIterate.state.lastTime) / 1_000_000_000.0;
    ScreenIterate.state.lastTime = now;
  }

  private static init(): void {
    ScreenIterate.state.wakeUp = 0;
    ScreenIterate.state.lastTime = process.hrtime.bigint();
    Statistics.instance().rayTracer.resetCounters();
  }

  private static finish(): void {
    ScreenIterate.updateCpuSecs();
  }

  public static sequential(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    callback: SCREEN_ITERATE_CALLBACK,
    data: unknown,
    toneMapOptions: ToneMappingContext
  ): void {
    ScreenIterate.init();

    const width = camera.xSize;
    const height = camera.ySize;
    const rgb = new Array<ColorRgb>(width);
    for (let i = 0; i < width; i++) {
      rgb[i] = new ColorRgb();
    }

    for (let i = 0; i < height; i++) {
      for (let j = 0; j < width; j++) {
        const col = callback.call(camera, sceneVoxelGrid, sceneBackground, j, i, data);
        ToneMap.radianceToRgb(col, rgb[j]!, toneMapOptions);
        Statistics.instance().rayTracer.pixelCount++;
      }

      SoftIds.softRenderPixels(width, 1, rgb, toneMapOptions);
    }

    ScreenIterate.finish();
  }

  private static fillRect(
    camera: Camera,
    x0: number,
    y0: number,
    x1: number,
    y1: number,
    col: ColorRgb,
    rgb: ColorRgb[]
  ): void {
    for (let x = x0; x < x1; x++) {
      for (let y = y0; y < y1; y++) {
        const c = rgb[y * camera.xSize + x];
        c!.set(col.r, col.g, col.b);
      }
    }
  }

  private static renderSegment(
    width: number,
    yMin: number,
    yMax: number,
    rgb: ColorRgb[],
    toneMapOptions: ToneMappingContext
  ): void {
    const height = yMax - yMin;
    if (height <= 0) {
      return;
    }
    const segment = new Array<ColorRgb>(width * height);
    const srcOffset = yMin * width;
    for (let i = 0; i < segment.length; i++) {
      const src = rgb[srcOffset + i];
      segment[i] = new ColorRgb(src!.r, src!.g, src!.b);
    }
    SoftIds.softRenderPixels(width, height, segment, toneMapOptions);
  }

  public static progressive(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    callback: SCREEN_ITERATE_CALLBACK,
    data: unknown,
    toneMapOptions: ToneMappingContext
  ): void {
    let x0: number;
    let y0: number;
    let x1: number;
    let y1: number;
    let stepSize: number;
    let xSteps: number;
    let ySteps: number;
    let xStepDone: boolean;
    let yStepDone: boolean;
    let skip: boolean;
    let yMin: number;
    let yMax: number;

    ScreenIterate.init();

    const width = camera.xSize;
    const height = camera.ySize;
    const pixelRGB = new ColorRgb();
    const rgb = new Array<ColorRgb>(width * height);

    const white = new ColorRgb(1.0, 1.0, 1.0);

    for (let i = 0; i < width * height; i++) {
      rgb[i] = new ColorRgb(white.r, white.g, white.b);
    }

    stepSize = 64;
    skip = false;
    yMin = height + 1;
    yMax = -1;

    while (stepSize > 0) {
      y0 = 0;
      ySteps = 0;
      yStepDone = false;

      while (!yStepDone) {
        y1 = y0 + stepSize;
        if (y1 >= height) {
          y1 = height;
          yStepDone = true;
        }

        yMin = globalThis.Math.min(y0, yMin);
        yMax = globalThis.Math.max(y1, yMax);

        x0 = 0;
        xSteps = 0;
        xStepDone = false;

        while (!xStepDone) {
          x1 = x0 + stepSize;

          if (x1 >= width) {
            x1 = width;
            xStepDone = true;
          }

          if (!skip || ((ySteps & 1) !== 0) || ((xSteps & 1) !== 0)) {
            const col = callback.call(camera, sceneVoxelGrid, sceneBackground, x0, height - y0 - 1, data);
            ToneMap.radianceToRgb(col, pixelRGB, toneMapOptions);
            ScreenIterate.fillRect(camera, x0, y0, x1, y1, pixelRGB, rgb);

            Statistics.instance().rayTracer.pixelCount++;

            if ((ScreenIterate.state.wakeUp & ScreenIterate.wakeUpRender()) !== 0) {
              ScreenIterate.state.wakeUp = ScreenIterate.state.wakeUp & (~ScreenIterate.wakeUpRender());
              if (yMax > 0 && yMax > yMin) {
                ScreenIterate.renderSegment(width, yMin, yMax, rgb, toneMapOptions);
              }
              yMin = globalThis.Math.max(0, yMax - stepSize);
            }
          }

          x0 = x1;
          xSteps++;
        }

        if (yMax >= height) {
          if (yMax > yMin) {
            ScreenIterate.renderSegment(width, yMin, yMax, rgb, toneMapOptions);
          }
          yMax = -1;
        }

        y0 = y1;
        ySteps++;
      }

      skip = true;
      stepSize /= 2;
    }

    ScreenIterate.finish();
  }
}
