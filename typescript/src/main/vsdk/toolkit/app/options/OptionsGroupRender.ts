import { ColorRgb } from "../../common/color/ColorRgb";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";

export class OptionsGroupRender {
  private static trueValue = 1;
  private static renderOptionsState = new RendererConfiguration();
  private static outlineColor = new ColorRgb();

  private constructor() {
  }

  private static flatOption(_value: TypedOption.MutableValue<number>): void {
    OptionsGroupRender.renderOptionsState.smoothShading = false;
  }

  private static noCullingOption(_value: TypedOption.MutableValue<number>): void {
    OptionsGroupRender.renderOptionsState.backfaceCulling = false;
  }

  private static outlinesOption(_value: TypedOption.MutableValue<number>): void {
    OptionsGroupRender.renderOptionsState.drawOutlines = true;
  }

  private static traceOption(_value: TypedOption.MutableValue<number>): void {
    OptionsGroupRender.renderOptionsState.trace = true;
  }

  private static parseColor3(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<ColorRgb>
  ): boolean {
    if (argc < 3 || argv === null || argv[0] === null || argv[1] === null || argv[2] === null) {
      return false;
    }
    const r = Number.parseFloat(argv[0]);
    const g = Number.parseFloat(argv[1]);
    const b = Number.parseFloat(argv[2]);
    if (Number.isNaN(r) || Number.isNaN(g) || Number.isNaN(b)) {
      return false;
    }
    value.value.r = r;
    value.value.g = g;
    value.value.b = b;
    return true;
  }

  private static copyFrom(source: RendererConfiguration | null, target: RendererConfiguration | null): void {
    if (source === null || target === null) {
      return;
    }

    target.outlineColor = new ColorRgb(source.outlineColor.r, source.outlineColor.g, source.outlineColor.b);
    target.boundingBoxColor = new ColorRgb(source.boundingBoxColor.r, source.boundingBoxColor.g, source.boundingBoxColor.b);
    target.clusterColor = new ColorRgb(source.clusterColor.r, source.clusterColor.g, source.clusterColor.b);
    target.lineWidth = source.lineWidth;
    target.drawOutlines = source.drawOutlines;
    target.drawSurfaces = source.drawSurfaces;
    target.noShading = source.noShading;
    target.smoothShading = source.smoothShading;
    target.backfaceCulling = source.backfaceCulling;
    target.drawBoundingBoxes = source.drawBoundingBoxes;
    target.drawClusters = source.drawClusters;
    target.frustumCulling = source.frustumCulling;
    target.renderRayTracedImage = source.renderRayTracedImage;
    target.trace = source.trace;
  }

  public static renderParseOptions(argc: number[], argv: string[], renderOptions: RendererConfiguration): void {
    const flatOpt = new TypedOption<number>(
      "-flat-shading",
      TypedOption.valueRef(OptionsGroupRender.trueValue),
      0,
      OptionsGroupRender.flatOption,
      null
    );
    const raycastOpt = new TypedOption<number>(
      "-raycast",
      TypedOption.valueRef(OptionsGroupRender.trueValue),
      0,
      OptionsGroupRender.traceOption,
      null
    );
    const noCullingOpt = new TypedOption<number>(
      "-no-culling",
      TypedOption.valueRef(OptionsGroupRender.trueValue),
      0,
      OptionsGroupRender.noCullingOption,
      null
    );
    const outlinesOpt = new TypedOption<number>(
      "-outlines",
      TypedOption.valueRef(OptionsGroupRender.trueValue),
      0,
      OptionsGroupRender.outlinesOption,
      null
    );
    const outlineColorOpt = new TypedOption<ColorRgb>(
      "-outline-color",
      TypedOption.valueRef(OptionsGroupRender.outlineColor),
      3,
      null,
      OptionsGroupRender.parseColor3
    );
    const renderingOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(flatOpt, 5),
      TypedOption.REGISTER_OPTION(raycastOpt, 5),
      TypedOption.REGISTER_OPTION(noCullingOpt, 5),
      TypedOption.REGISTER_OPTION(outlinesOpt, 5),
      TypedOption.REGISTER_OPTION(outlineColorOpt, 10),
    ];

    OptionsGroupRender.copyFrom(renderOptions, OptionsGroupRender.renderOptionsState);
    const renderGroups = [
      new OptionGroup("render", renderingOptions, 5),
    ];
    OptionParser.parse(argc, argv, renderGroups, 1);

    OptionsGroupRender.copyFrom(OptionsGroupRender.renderOptionsState, renderOptions);
    renderOptions.outlineColor.r = OptionsGroupRender.outlineColor.r;
    renderOptions.outlineColor.g = OptionsGroupRender.outlineColor.g;
    renderOptions.outlineColor.b = OptionsGroupRender.outlineColor.b;
  }
}
