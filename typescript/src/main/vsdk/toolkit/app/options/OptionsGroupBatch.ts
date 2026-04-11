import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { BatchOptions } from "./BatchOptions";

export class OptionsGroupBatch {
  private static batchOptionsState = new BatchOptions();

  private constructor() {
  }

  public static parse(argc: number[], argv: string[], batchOptions: BatchOptions): void {
    OptionsGroupBatch.batchParseOptions(argc, argv, batchOptions);
  }

  private static binaryOutputOption(_value: TypedOption.MutableValue<string>): void {
    OptionsGroupBatch.batchOptionsState.exportBinary =
      OptionsGroupBatch.batchOptionsState.binaryOutputFilename !== null
      && OptionsGroupBatch.batchOptionsState.binaryOutputFilename.length > 0;
  }

  private static binaryInputOption(_value: TypedOption.MutableValue<string>): void {
    OptionsGroupBatch.batchOptionsState.importBinary =
      OptionsGroupBatch.batchOptionsState.binaryInputFilename !== null
      && OptionsGroupBatch.batchOptionsState.binaryInputFilename.length > 0;
  }

  private static setIntTrue(value: TypedOption.MutableValue<number>): void {
    value.value = 1;
  }

  private static copyFrom(source: BatchOptions | null, target: BatchOptions | null): void {
    if (source === null || target === null) {
      return;
    }

    target.exportBinary = source.exportBinary;
    target.binaryOutputFilename = source.binaryOutputFilename;
    target.importBinary = source.importBinary;
    target.binaryInputFilename = source.binaryInputFilename;
    target.iterations = source.iterations;
    target.radianceImageFileNameFormat = source.radianceImageFileNameFormat;
    target.radianceModelFileNameFormat = source.radianceModelFileNameFormat;
    target.saveModulo = source.saveModulo;
    target.raytracingImageFileName = source.raytracingImageFileName;
    target.timings = source.timings;
  }

  public static batchParseOptions(argc: number[], argv: string[], options: BatchOptions): void {
    const iterationsOpt = new TypedOption<number>(
      "-iterations",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.iterations,
        (v) => {
          OptionsGroupBatch.batchOptionsState.iterations = v;
        }
      ),
      1,
      null,
      null
    );
    const obfOpt = new TypedOption<string>(
      "-obf",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.binaryOutputFilename,
        (v) => {
          OptionsGroupBatch.batchOptionsState.binaryOutputFilename = v;
        }
      ),
      1,
      OptionsGroupBatch.binaryOutputOption,
      null
    );
    const ibfOpt = new TypedOption<string>(
      "-ibf",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.binaryInputFilename,
        (v) => {
          OptionsGroupBatch.batchOptionsState.binaryInputFilename = v;
        }
      ),
      1,
      OptionsGroupBatch.binaryInputOption,
      null
    );
    const radianceImageOpt = new TypedOption<string>(
      "-radiance-image-savefile",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.radianceImageFileNameFormat,
        (v) => {
          OptionsGroupBatch.batchOptionsState.radianceImageFileNameFormat = v;
        }
      ),
      1,
      null,
      null
    );
    const radianceModelOpt = new TypedOption<string>(
      "-radiance-model-savefile",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.radianceModelFileNameFormat,
        (v) => {
          OptionsGroupBatch.batchOptionsState.radianceModelFileNameFormat = v;
        }
      ),
      1,
      null,
      null
    );
    const saveModuloOpt = new TypedOption<number>(
      "-save-modulo",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.saveModulo,
        (v) => {
          OptionsGroupBatch.batchOptionsState.saveModulo = v;
        }
      ),
      1,
      null,
      null
    );
    const raytracingImageOpt = new TypedOption<string>(
      "-raytracing-image-savefile",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.raytracingImageFileName,
        (v) => {
          OptionsGroupBatch.batchOptionsState.raytracingImageFileName = v;
        }
      ),
      1,
      null,
      null
    );
    const timingsOpt = new TypedOption<number>(
      "-timings",
      TypedOption.reference(
        () => OptionsGroupBatch.batchOptionsState.timings,
        (v) => {
          OptionsGroupBatch.batchOptionsState.timings = v;
        }
      ),
      0,
      OptionsGroupBatch.setIntTrue,
      null
    );

    const batchCommandLineOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(iterationsOpt, 3),
      TypedOption.REGISTER_OPTION(obfOpt, 4),
      TypedOption.REGISTER_OPTION(ibfOpt, 4),
      TypedOption.REGISTER_OPTION(radianceImageOpt, 12),
      TypedOption.REGISTER_OPTION(radianceModelOpt, 12),
      TypedOption.REGISTER_OPTION(saveModuloOpt, 8),
      TypedOption.REGISTER_OPTION(raytracingImageOpt, 14),
      TypedOption.REGISTER_OPTION(timingsOpt, 3),
    ];
    const batchGroups = [
      new OptionGroup("batch", batchCommandLineOptions, 8),
    ];

    OptionsGroupBatch.copyFrom(options, OptionsGroupBatch.batchOptionsState);
    OptionsGroupBatch.batchOptionsState.exportBinary = false;
    OptionsGroupBatch.batchOptionsState.importBinary = false;
    OptionParser.parse(argc, argv, batchGroups, 1);
    OptionsGroupBatch.copyFrom(OptionsGroupBatch.batchOptionsState, options);
  }
}
