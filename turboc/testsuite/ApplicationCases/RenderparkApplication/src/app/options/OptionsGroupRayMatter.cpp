#include <strings.h>

#include "vsdk/common/commandLineOptions/OptionParser.h"
#include "vsdk/common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRayMatter.h"

template<typename T>
bool OptionsGroupRayMatter::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
    if ( argc < 1 || argv == NULL || argv[0] == NULL || binding.target == NULL || binding.values == NULL ) {
        return false;
    }
    for ( int i = 0; binding.values[i].name != NULL; i++ ) {
        if ( strncasecmp(argv[0], binding.values[i].name, binding.values[i].abbrev) == 0 ) {
            *binding.target = ((T)(binding.values[i].value));
            return true;
        }
    }
    return false;
}

EnumDesc OptionsGroupRayMatter::rayMatterPixelFilterValues[] = {
    {BOX_FILTER, "box", 2},
    {TENT_FILTER, "tent", 2},
    {GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, NULL, 0}
};

void
OptionsGroupRayMatter::rayMattingParseOptions(
        int *argc,
        char **argv,
        RayMatterState &rayMatterState)
{
    EnumBinding<RayMatterFilterType> pixelFilterBinding = {&rayMatterState.filter, rayMatterPixelFilterValues};
    TypedOption<int> rmSamplesOpt("-rm-samples-per-pixel", &rayMatterState.samplesPerPixel, 1, NULL, NULL);
    TypedOption<EnumBinding<RayMatterFilterType> > rmPixelFilterOpt("-rm-pixel-filter", &pixelFilterBinding, 1, NULL, parseEnumBinding<RayMatterFilterType>);
    OptionBase rayMatterOptions[] = {
        REGISTER_OPTION(int, rmSamplesOpt, 6),
        REGISTER_OPTION(EnumBinding<RayMatterFilterType>, rmPixelFilterOpt, 7)
    };
    OptionGroup rayMatterGroups[] = {
        OptionGroup("rayMatter", rayMatterOptions, 2)
    };
    OptionParser<OptionBase>::parse(argc, argv, rayMatterGroups, 1);
}
