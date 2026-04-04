#include <cstring>
#include <strings.h>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupPhotonMap.h"

bool
OptionsGroupPhotonMap::parseBoolInt(int argc, char **argv, int &value) {
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
OptionsGroupPhotonMap::setIntTrue(int &value) {
    value = 1;
}

void
OptionsGroupPhotonMap::parse(
        int *argc,
        char **argv,
        PhotonMapState &photonMapState)
{
    TypedOption<int> pmapDoGlobalOpt = {"-pmap-do-global", &photonMapState.doGlobalMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapGlobalPathsOpt = {"-pmap-global-paths", &photonMapState.gPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapGPreirradianceOpt = {"-pmap-g-preirradiance", &photonMapState.precomputeGIrradiance, 1, nullptr, parseBoolInt};
    TypedOption<int> pmapDoCausticOpt = {"-pmap-do-caustic", &photonMapState.doCausticMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapCausticPathsOpt = {"-pmap-caustic-paths", &photonMapState.cPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapRenderHitsOpt = {"-pmap-render-hits", &photonMapState.renderImage, 0, setIntTrue, nullptr};
    TypedOption<int> pmapReconGPhotonsOpt = {"-pmap-recon-gphotons", &photonMapState.reconGPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconIPhotonsOpt = {"-pmap-recon-iphotons", &photonMapState.reconCPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconPhotonsOpt = {"-pmap-recon-photons", &photonMapState.reconIPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapBalancingOpt = {"-pmap-balancing", &photonMapState.balanceKDTree, 1, nullptr, parseBoolInt};
    OptionBase photonMapOptions[] = {
        REGISTER_OPTION(int, pmapDoGlobalOpt, 9),
        REGISTER_OPTION(long, pmapGlobalPathsOpt, 9),
        REGISTER_OPTION(int, pmapGPreirradianceOpt, 11),
        REGISTER_OPTION(int, pmapDoCausticOpt, 9),
        REGISTER_OPTION(long, pmapCausticPathsOpt, 9),
        REGISTER_OPTION(int, pmapRenderHitsOpt, 9),
        REGISTER_OPTION(int, pmapReconGPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconIPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapBalancingOpt, 9)
    };
    OptionGroup photonMapGroups[] = {
        OptionGroup("photonMap", photonMapOptions, 10)
    };
    OptionParser<OptionBase>::parse(argc, argv, photonMapGroups, 1);
}
