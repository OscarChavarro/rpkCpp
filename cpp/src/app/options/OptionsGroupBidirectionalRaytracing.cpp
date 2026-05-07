#include <cstring>
#include <cstring>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "app/options/OptionsGroupBidirectionalRaytracing.h"

int OptionsGroupBidirectionalRaytracing::regExpStringLength = BidirectionalPathRaytracerConfig::MAX_REGEXP_SIZE;

bool
OptionsGroupBidirectionalRaytracing::parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.maxLength <= 0 ) {
        return false;
    }
    strncpy(binding.target, argv[0], binding.maxLength);
    binding.target[binding.maxLength - 1] = '\0';
    return true;
}

bool
OptionsGroupBidirectionalRaytracing::parseBoolInt(int argc, char **argv, int &value) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr ) {
        return false;
    }
    if ( strcasecmp(argv[0], "true") == 0 || strcasecmp(argv[0], "yes") == 0 || strcmp(argv[0], "1") == 0 ) {
        value = 1;
        return true;
    }
    if ( strcasecmp(argv[0], "false") == 0 || strcasecmp(argv[0], "no") == 0 || strcmp(argv[0], "0") == 0 ) {
        value = 0;
        return true;
    }
    return false;
}

void
OptionsGroupBidirectionalRaytracing::setIntTrue(int &value) {
    value = 1;
}

void
OptionsGroupBidirectionalRaytracing::setIntFalse(int &value) {
    value = 0;
}

void
OptionsGroupBidirectionalRaytracing::parse(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState)
{
    FixedStringBinding leBinding = {bidirectionalPathState.baseConfig.leRegExp, regExpStringLength};
    FixedStringBinding ldBinding = {bidirectionalPathState.baseConfig.ldRegExp, regExpStringLength};
    FixedStringBinding liBinding = {bidirectionalPathState.baseConfig.liRegExp, regExpStringLength};
    TypedOption<int> bidirSamplesPerPixelOpt = {"-bidir-samples-per-pixel", &bidirectionalPathState.baseConfig.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<int> bidirNoProgressiveOpt = {"-bidir-no-progressive", &bidirectionalPathState.baseConfig.progressiveTracing, 0, setIntFalse, nullptr};
    TypedOption<int> bidirMaxEyePathLengthOpt = {"-bidir-max-eye-path-length", &bidirectionalPathState.baseConfig.maximumEyePathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxLightPathLengthOpt = {"-bidir-max-light-path-length", &bidirectionalPathState.baseConfig.maximumLightPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxPathLengthOpt = {"-bidir-max-path-length", &bidirectionalPathState.baseConfig.maximumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMinPathLengthOpt = {"-bidir-min-path-length", &bidirectionalPathState.baseConfig.minimumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirNoLightImportanceOpt = {"-bidir-no-light-importance", &bidirectionalPathState.baseConfig.sampleImportantLights, 0, setIntFalse, nullptr};
    TypedOption<int> bidirUseRegexpOpt = {"-bidir-use-regexp", &bidirectionalPathState.baseConfig.useSpars, 0, setIntTrue, nullptr};
    TypedOption<int> bidirUseEmittedOpt = {"-bidir-use-emitted", &bidirectionalPathState.baseConfig.doLe, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpEmittedOpt = {"-bidir-rexp-emitted", &leBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegDirectOpt = {"-bidir-reg-direct", &bidirectionalPathState.baseConfig.doLD, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpDirectOpt = {"-bidir-rexp-direct", &ldBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegIndirectOpt = {"-bidir-reg-indirect", &bidirectionalPathState.baseConfig.doLI, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpIndirectOpt = {"-bidir-rexp-indirect", &liBinding, 1, nullptr, parseFixedStringBinding};
    OptionBase bidirectionalOptions[] = {
        REGISTER_OPTION(int, bidirSamplesPerPixelOpt, 8),
        REGISTER_OPTION(int, bidirNoProgressiveOpt, 11),
        REGISTER_OPTION(int, bidirMaxEyePathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxLightPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMinPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirNoLightImportanceOpt, 11),
        REGISTER_OPTION(int, bidirUseRegexpOpt, 12),
        REGISTER_OPTION(int, bidirUseEmittedOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpEmittedOpt, 13),
        REGISTER_OPTION(int, bidirRegDirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpDirectOpt, 13),
        REGISTER_OPTION(int, bidirRegIndirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpIndirectOpt, 13)
    };
    OptionGroup bidirectionalGroups[] = {
        OptionGroup("bidirectional", bidirectionalOptions, 14)
    };
    OptionParser<OptionBase>::parse(argc, argv, bidirectionalGroups, 1);
}
