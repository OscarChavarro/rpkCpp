#include "common/commandLineOptions/OptionParser.h"
#include "app/options/CommandLine.h"

namespace {

void setIntTrue(int &value) {
    value = 1;
}

}

void
CommandLine::binaryOutputOption(const char *& /*value*/) {
    batchOptionsState.exportBinary =
        batchOptionsState.binaryOutputFilename != nullptr
        && batchOptionsState.binaryOutputFilename[0] != '\0';
}

void
CommandLine::binaryInputOption(const char *& /*value*/) {
    batchOptionsState.importBinary =
        batchOptionsState.binaryInputFilename != nullptr
        && batchOptionsState.binaryInputFilename[0] != '\0';
}

void
CommandLine::batchParseOptions(
        int *argc,
        char **argv,
        BatchOptions *options)
{
    TypedOption<int> iterationsOpt = {"-iterations", &batchOptionsState.iterations, 1, nullptr, nullptr};
    TypedOption<const char *> obfOpt = {"-obf", &batchOptionsState.binaryOutputFilename, 1, CommandLine::binaryOutputOption, nullptr};
    TypedOption<const char *> ibfOpt = {"-ibf", &batchOptionsState.binaryInputFilename, 1, CommandLine::binaryInputOption, nullptr};
    TypedOption<const char *> radianceImageOpt = {"-radiance-image-savefile", &batchOptionsState.radianceImageFileNameFormat, 1, nullptr, nullptr};
    TypedOption<const char *> radianceModelOpt = {"-radiance-model-savefile", &batchOptionsState.radianceModelFileNameFormat, 1, nullptr, nullptr};
    TypedOption<int> saveModuloOpt = {"-save-modulo", &batchOptionsState.saveModulo, 1, nullptr, nullptr};
    TypedOption<const char *> raytracingImageOpt = {"-raytracing-image-savefile", &batchOptionsState.raytracingImageFileName, 1, nullptr, nullptr};
    TypedOption<int> timingsOpt = {"-timings", &batchOptionsState.timings, 0, setIntTrue, nullptr};
    OptionBase batchCommandLineOptions[] = {
        REGISTER_OPTION(int, iterationsOpt, 3),
        REGISTER_OPTION(const char *, obfOpt, 4),
        REGISTER_OPTION(const char *, ibfOpt, 4),
        REGISTER_OPTION(const char *, radianceImageOpt, 12),
        REGISTER_OPTION(const char *, radianceModelOpt, 12),
        REGISTER_OPTION(int, saveModuloOpt, 8),
        REGISTER_OPTION(const char *, raytracingImageOpt, 14),
        REGISTER_OPTION(int, timingsOpt, 3)
    };
    OptionGroup batchGroups[] = {
        OptionGroup("batch", batchCommandLineOptions, 8)
    };

    batchOptionsState = *options;
    batchOptionsState.exportBinary = false;
    batchOptionsState.importBinary = false;
    OptionParser<OptionBase>::parse(argc, argv, batchGroups, 1);
    *options = batchOptionsState;
}
