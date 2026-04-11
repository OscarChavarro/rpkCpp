import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";

export class OptionsGroupRaytracingMethod {
  private static raytracingMethodsString: string | null = null;
  private static rayTracerName: string[] | null = null;

  private constructor() {
  }

  private static mainRayTracingOption(name: TypedOption.MutableValue<string | null>): void {
    if (OptionsGroupRaytracingMethod.rayTracerName !== null && OptionsGroupRaytracingMethod.rayTracerName.length > 0) {
      OptionsGroupRaytracingMethod.rayTracerName[0] = name.value ?? "";
    }
  }

  public static rayTracingParseOptions(
    argc: number[],
    argv: string[],
    raytracingMethodsStringOut: string[],
    rayTracerNameOut: string[]
  ): void {
    const raytracingMethodName = TypedOption.valueRef<string | null>(null);
    const raytracingMethodOpt = new TypedOption<string | null>(
      "-raytracing-method",
      raytracingMethodName,
      1,
      OptionsGroupRaytracingMethod.mainRayTracingOption,
      null
    );
    const raytracingOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(raytracingMethodOpt, 4),
    ];

    OptionsGroupRaytracingMethod.rayTracerName = rayTracerNameOut;
    OptionsGroupRaytracingMethod.raytracingMethodsString =
      raytracingMethodsStringOut !== null && raytracingMethodsStringOut.length > 0
        ? raytracingMethodsStringOut[0]
        : null;
    const raytracingGroups = [
      new OptionGroup("raytracing", raytracingOptions, 1),
    ];
    OptionParser.parse(argc, argv, raytracingGroups, 1);
  }
}
