package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;

public final class OptionsGroupBatch {
    private static BatchOptions batchOptionsState = new BatchOptions();

    public static void parse(
        int[] argc,
        String[] argv,
        BatchOptions batchOptions)
    {
        OptionsGroupBatch.batchParseOptions(argc, argv, batchOptions);
    }

    private static void binaryOutputOption(TypedOption.MutableValue<String> value) {
        batchOptionsState.exportBinary =
            batchOptionsState.binaryOutputFilename != null
            && !batchOptionsState.binaryOutputFilename.isEmpty();
    }

    private static void binaryInputOption(TypedOption.MutableValue<String> value) {
        batchOptionsState.importBinary =
            batchOptionsState.binaryInputFilename != null
            && !batchOptionsState.binaryInputFilename.isEmpty();
    }

    private static void setIntTrue(TypedOption.MutableValue<Integer> value) {
        value.value = 1;
    }

    private static void copyFrom(BatchOptions source, BatchOptions target) {
        if (source == null || target == null) {
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

    public static void batchParseOptions(
        int[] argc,
        String[] argv,
        BatchOptions options)
    {
        TypedOption<Integer> iterationsOpt = new TypedOption<>(
            "-iterations",
            TypedOption.reference(() -> batchOptionsState.iterations, v -> batchOptionsState.iterations = v),
            1,
            null,
            null);
        TypedOption<String> obfOpt = new TypedOption<>(
            "-obf",
            TypedOption.reference(() -> batchOptionsState.binaryOutputFilename, v -> batchOptionsState.binaryOutputFilename = v),
            1,
            OptionsGroupBatch::binaryOutputOption,
            null);
        TypedOption<String> ibfOpt = new TypedOption<>(
            "-ibf",
            TypedOption.reference(() -> batchOptionsState.binaryInputFilename, v -> batchOptionsState.binaryInputFilename = v),
            1,
            OptionsGroupBatch::binaryInputOption,
            null);
        TypedOption<String> radianceImageOpt = new TypedOption<>(
            "-radiance-image-savefile",
            TypedOption.reference(() -> batchOptionsState.radianceImageFileNameFormat, v -> batchOptionsState.radianceImageFileNameFormat = v),
            1,
            null,
            null);
        TypedOption<String> radianceModelOpt = new TypedOption<>(
            "-radiance-model-savefile",
            TypedOption.reference(() -> batchOptionsState.radianceModelFileNameFormat, v -> batchOptionsState.radianceModelFileNameFormat = v),
            1,
            null,
            null);
        TypedOption<Integer> saveModuloOpt = new TypedOption<>(
            "-save-modulo",
            TypedOption.reference(() -> batchOptionsState.saveModulo, v -> batchOptionsState.saveModulo = v),
            1,
            null,
            null);
        TypedOption<String> raytracingImageOpt = new TypedOption<>(
            "-raytracing-image-savefile",
            TypedOption.reference(() -> batchOptionsState.raytracingImageFileName, v -> batchOptionsState.raytracingImageFileName = v),
            1,
            null,
            null);
        TypedOption<Integer> timingsOpt = new TypedOption<>(
            "-timings",
            TypedOption.reference(() -> batchOptionsState.timings, v -> batchOptionsState.timings = v),
            0,
            OptionsGroupBatch::setIntTrue,
            null);
        OptionBase[] batchCommandLineOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(iterationsOpt, 3),
            TypedOption.REGISTER_OPTION(obfOpt, 4),
            TypedOption.REGISTER_OPTION(ibfOpt, 4),
            TypedOption.REGISTER_OPTION(radianceImageOpt, 12),
            TypedOption.REGISTER_OPTION(radianceModelOpt, 12),
            TypedOption.REGISTER_OPTION(saveModuloOpt, 8),
            TypedOption.REGISTER_OPTION(raytracingImageOpt, 14),
            TypedOption.REGISTER_OPTION(timingsOpt, 3)
        };
        OptionGroup[] batchGroups = new OptionGroup[] {
            new OptionGroup("batch", batchCommandLineOptions, 8)
        };

        copyFrom(options, batchOptionsState);
        batchOptionsState.exportBinary = false;
        batchOptionsState.importBinary = false;
        OptionParser.parse(argc, argv, batchGroups, 1);
        copyFrom(batchOptionsState, options);
    }
}
