#include <strings.h>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRayMatter.h"

template<typename T>
bool OptionsGroupRayMatter::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.values == nullptr ) {
        return false;
    }
    for ( int i = 0; binding.values[i].name != nullptr; i++ ) {
        if ( strncasecmp(argv[0], binding.values[i].name, binding.values[i].abbrev) == 0 ) {
            *binding.target = static_cast<T>(binding.values[i].value);
            return true;
        }
    }
    return false;
}

EnumDesc OptionsGroupRayMatter::rayMatterPixelFilterValues[] = {
    {RayMatterFilterType::BOX_FILTER, "box", 2},
    {RayMatterFilterType::TENT_FILTER, "tent", 2},
    {RayMatterFilterType::GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RayMatterFilterType::GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};

void
OptionsGroupRayMatter::rayMattingParseOptions(
        int *argc,
        char **argv,
        RayMatterState &rayMatterState)
{
    EnumBinding<RayMatterFilterType> pixelFilterBinding = {&rayMatterState.filter, rayMatterPixelFilterValues};
    TypedOption<int> rmSamplesOpt = {"-rm-samples-per-pixel", &rayMatterState.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<EnumBinding<RayMatterFilterType>> rmPixelFilterOpt = {"-rm-pixel-filter", &pixelFilterBinding, 1, nullptr, parseEnumBinding<RayMatterFilterType>};
    OptionBase rayMatterOptions[] = {
        REGISTER_OPTION(int, rmSamplesOpt, 6),
        REGISTER_OPTION(EnumBinding<RayMatterFilterType>, rmPixelFilterOpt, 7)
    };

    OptionGroup rayMatterGroups[] = {
        OptionGroup("rayMatter", rayMatterOptions, 2)
    };
    OptionParser<OptionBase>::parse(argc, argv, rayMatterGroups, 1);
}
