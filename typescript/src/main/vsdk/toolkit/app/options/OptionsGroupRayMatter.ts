import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { RayMatterFilterType } from "../../raycasting/simple/RayMatterFilterType";
import { RayMatterState } from "../../raycasting/simple/RayMatterState";
import { EnumDesc } from "./EnumDesc";
import { OptionTextUtils } from "./OptionTextUtils";

type EnumBinding<T extends number> = {
  target: TypedOption.Reference<T>;
  values: EnumDesc[];
  enumType: Record<string, number | string>;
};

export class OptionsGroupRayMatter {
  private static rayMatterPixelFilterValues: EnumDesc[] = [
    new EnumDesc(RayMatterFilterType.BOX_FILTER, "box", 2),
    new EnumDesc(RayMatterFilterType.TENT_FILTER, "tent", 2),
    new EnumDesc(RayMatterFilterType.GAUSS_FILTER, "gaussian 1/sqrt2", 2),
    new EnumDesc(RayMatterFilterType.GAUSS2_FILTER, "gaussian 1/2", 2),
    new EnumDesc(0, null, 0),
  ];

  private constructor() {
  }

  private static parseEnumBinding<T extends number>(
    argc: number,
    argv: string[] | null,
    bindingValue: TypedOption.MutableValue<EnumBinding<T>>
  ): boolean {
    const binding = bindingValue.value;
    if (argc < 1 || argv === null || argv[0] === null || binding === null || binding.target === null || binding.values === null) {
      return false;
    }
    for (let i = 0; binding.values[i].name !== null; i++) {
      if (OptionTextUtils.equalsIgnoreCasePrefix(argv[0], binding.values[i].name, binding.values[i].abbrev)) {
        const ordinal = binding.values[i].value;
        if ((binding.enumType as Record<number, string | number>)[ordinal] === undefined) {
          return false;
        }
        binding.target.set(ordinal as T);
        return true;
      }
    }
    return false;
  }

  public static rayMattingParseOptions(argc: number[], argv: string[], rayMatterState: RayMatterState): void {
    const pixelFilterBinding: EnumBinding<RayMatterFilterType> = {
      target: TypedOption.reference(() => rayMatterState.filter, (v) => {
        rayMatterState.filter = v;
      }),
      values: OptionsGroupRayMatter.rayMatterPixelFilterValues,
      enumType: RayMatterFilterType as unknown as Record<string, number | string>,
    };

    const rmSamplesOpt = new TypedOption<number>(
      "-rm-samples-per-pixel",
      TypedOption.reference(() => rayMatterState.samplesPerPixel, (v) => {
        rayMatterState.samplesPerPixel = v;
      }),
      1,
      null,
      null
    );
    const rmPixelFilterOpt = new TypedOption<EnumBinding<RayMatterFilterType>>(
      "-rm-pixel-filter",
      TypedOption.valueRef(pixelFilterBinding),
      1,
      null,
      OptionsGroupRayMatter.parseEnumBinding
    );
    const rayMatterOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(rmSamplesOpt, 6),
      TypedOption.REGISTER_OPTION(rmPixelFilterOpt, 7),
    ];

    const rayMatterGroups = [
      new OptionGroup("rayMatter", rayMatterOptions, 2),
    ];
    OptionParser.parse(argc, argv, rayMatterGroups, 1);
  }
}
