#include <string.h>
#include <strings.h>

#include "vsdk/common/commandLineOptions/OptionParser.h"
#include "vsdk/common/commandLineOptions/TypedOption.h"
#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "app/options/OptionsGroupBidirectionalRaytracing.h"

int OptsGrpBidirRaytr::regExpStringLength = MAX_REGEXP_SIZE;

bool
OptsGrpBidirRaytr::parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding) {
    if ( argc < 1 || argv == NULL || argv[0] == NULL || binding.target == NULL || binding.maxLength <= 0 ) {
        return false;
    }
    strncpy(binding.target, argv[0], binding.maxLength);
    binding.target[binding.maxLength - 1] = '\0';
    return true;
}

bool
OptsGrpBidirRaytr::parseBoolInt(int argc, char **argv, int &value) {
    if ( argc < 1 || argv == NULL || argv[0] == NULL ) {
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
OptsGrpBidirRaytr::setIntTrue(int &value) {
    value = 1;
}

void
OptsGrpBidirRaytr::setIntFalse(int &value) {
    value = 0;
}

void
OptsGrpBidirRaytr::parse(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState)
{
    FixedStringBinding leBinding = {bidirectionalPathState.baseConfig.leRegExp, regExpStringLength};
    FixedStringBinding ldBinding = {bidirectionalPathState.baseConfig.ldRegExp, regExpStringLength};
    FixedStringBinding liBinding = {bidirectionalPathState.baseConfig.liRegExp, regExpStringLength};
    TypedOption<int> bidirSamplesPerPixelOpt("-bidir-samples-per-pixel", &bidirectionalPathState.baseConfig.samplesPerPixel, 1, NULL, NULL);
    TypedOption<int> bidirNoProgressiveOpt("-bidir-no-progressive", &bidirectionalPathState.baseConfig.progressiveTracing, 0, setIntFalse, NULL);
    TypedOption<int> bidirMaxEyePathLengthOpt("-bidir-max-eye-path-length", &bidirectionalPathState.baseConfig.maximumEyePathDepth, 1, NULL, NULL);
    TypedOption<int> bidirMaxLightPathLengthOpt("-bidir-max-light-path-length", &bidirectionalPathState.baseConfig.maximumLightPathDepth, 1, NULL, NULL);
    TypedOption<int> bidirMaxPathLengthOpt("-bidir-max-path-length", &bidirectionalPathState.baseConfig.maximumPathDepth, 1, NULL, NULL);
    TypedOption<int> bidirMinPathLengthOpt("-bidir-min-path-length", &bidirectionalPathState.baseConfig.minimumPathDepth, 1, NULL, NULL);
    TypedOption<int> bidirNoLightImportanceOpt("-bidir-no-light-importance", &bidirectionalPathState.baseConfig.sampleImportantLights, 0, setIntFalse, NULL);
    TypedOption<int> bidirUseRegexpOpt("-bidir-use-regexp", &bidirectionalPathState.baseConfig.useSpars, 0, setIntTrue, NULL);
    TypedOption<int> bidirUseEmittedOpt("-bidir-use-emitted", &bidirectionalPathState.baseConfig.doLe, 1, NULL, parseBoolInt);
    TypedOption<FixedStringBinding> bidirRexpEmittedOpt("-bidir-rexp-emitted", &leBinding, 1, NULL, parseFixedStringBinding);
    TypedOption<int> bidirRegDirectOpt("-bidir-reg-direct", &bidirectionalPathState.baseConfig.doLD, 1, NULL, parseBoolInt);
    TypedOption<FixedStringBinding> bidirRexpDirectOpt("-bidir-rexp-direct", &ldBinding, 1, NULL, parseFixedStringBinding);
    TypedOption<int> bidirRegIndirectOpt("-bidir-reg-indirect", &bidirectionalPathState.baseConfig.doLI, 1, NULL, parseBoolInt);
    TypedOption<FixedStringBinding> bidirRexpIndirectOpt("-bidir-rexp-indirect", &liBinding, 1, NULL, parseFixedStringBinding);
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
