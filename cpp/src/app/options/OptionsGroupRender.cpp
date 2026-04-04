#include <cstdlib>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRender.h"

int OptionsGroupRender::trueValue = true;
RenderOptions OptionsGroupRender::renderOptionsState;
ColorRgb OptionsGroupRender::outlineColor;

void
OptionsGroupRender::flatOption(int & /*value*/) {
    renderOptionsState.smoothShading = false;
}

void
OptionsGroupRender::noCullingOption(int & /*value*/) {
    renderOptionsState.backfaceCulling = false;
}

void
OptionsGroupRender::outlinesOption(int & /*value*/) {
    renderOptionsState.drawOutlines = true;
}

void
OptionsGroupRender::traceOption(int & /*value*/) {
    renderOptionsState.trace = true;
}

void
OptionsGroupRender::renderParseOptions(
        int *argc,
        char **argv,
        RenderOptions *renderOptions)
{
    TypedOption<int> flatOpt = {"-flat-shading", &trueValue, 0, OptionsGroupRender::flatOption, nullptr};
    TypedOption<int> raycastOpt = {"-raycast", &trueValue, 0, OptionsGroupRender::traceOption, nullptr};
    TypedOption<int> noCullingOpt = {"-no-culling", &trueValue, 0, OptionsGroupRender::noCullingOption, nullptr};
    TypedOption<int> outlinesOpt = {"-outlines", &trueValue, 0, OptionsGroupRender::outlinesOption, nullptr};
    TypedOption<ColorRgb> outlineColorOpt = {"-outline-color", &outlineColor, 3, nullptr, OptionsGroupRender::parseColor3};
    OptionBase renderingOptions[] = {
        REGISTER_OPTION(int, flatOpt, 5),
        REGISTER_OPTION(int, raycastOpt, 5),
        REGISTER_OPTION(int, noCullingOpt, 5),
        REGISTER_OPTION(int, outlinesOpt, 5),
        REGISTER_OPTION(ColorRgb, outlineColorOpt, 10)
    };

    renderOptionsState = *renderOptions;
    OptionGroup renderGroups[] = {
        OptionGroup("render", renderingOptions, 5)
    };
    OptionParser<OptionBase>::parse(argc, argv, renderGroups, 1);

    *renderOptions = renderOptionsState;
    renderOptions->outlineColor.r = outlineColor.r;
    renderOptions->outlineColor.g = outlineColor.g;
    renderOptions->outlineColor.b = outlineColor.b;
}

bool
OptionsGroupRender::parseColor3(int argc, char **argv, ColorRgb &value) {
    if ( argc < 3 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr || argv[2] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
    value.r = strtof(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    value.g = strtof(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    value.b = strtof(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    return true;
}
