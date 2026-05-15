import { OptionBase } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionBase";
import { OptionGroup } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionGroup";
import { OptionParser } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionParser";
import { TypedOption } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/TypedOption";

export class OptionsGroupRadianceMethod {
  private constructor() {
  }

  public static radianceMethodParseOptions(
    argc: number[],
    argv: string[],
    radianceMethodsStringOut: string[]
  ): void {
    const radianceMethodName = TypedOption.valueRef<string | null>(null);
    const radianceMethodOpt = new TypedOption<string | null>(
      "-radiance-method",
      radianceMethodName,
      1,
      null,
      null
    );
    const radianceOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(radianceMethodOpt, 4),
    ];
    const radianceGroups = [
      new OptionGroup("radiance", radianceOptions, 1),
    ];
    OptionParser.parse(argc, argv, radianceGroups, 1);

    if (radianceMethodsStringOut !== null && radianceMethodsStringOut.length > 0) {
      radianceMethodsStringOut[0] = radianceMethodName.get() !== null ? radianceMethodName.get() as string : "";
    }
  }
}
