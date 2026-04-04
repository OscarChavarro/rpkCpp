package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.photonMap.PhotonMapState;

public final class OptionsGroupPhotonMap {
    private static boolean parseBoolInt(
        int argc,
        String[] argv,
        TypedOption.MutableValue<Integer> value)
    {
        if (argc < 1 || argv == null || argv[0] == null) {
            return false;
        }
        OptionTextUtils.TypedIntValue parsed = new OptionTextUtils.TypedIntValue(value.value);
        if (!OptionTextUtils.parseBoolInt(argv[0], parsed)) {
            return false;
        }
        value.value = parsed.value;
        return true;
    }

    private static void setIntTrue(TypedOption.MutableValue<Integer> value) {
        value.value = 1;
    }

    public static void parse(
        int[] argc,
        String[] argv,
        PhotonMapState photonMapState)
    {
        TypedOption<Integer> pmapDoGlobalOpt = new TypedOption<>(
            "-pmap-do-global",
            TypedOption.reference(() -> photonMapState.doGlobalMap, v -> photonMapState.doGlobalMap = v),
            1,
            null,
            OptionsGroupPhotonMap::parseBoolInt);
        TypedOption<Long> pmapGlobalPathsOpt = new TypedOption<>(
            "-pmap-global-paths",
            TypedOption.reference(() -> photonMapState.gPathsPerIteration, v -> photonMapState.gPathsPerIteration = v),
            1,
            null,
            null);
        TypedOption<Integer> pmapGPreirradianceOpt = new TypedOption<>(
            "-pmap-g-preirradiance",
            TypedOption.reference(() -> photonMapState.precomputeGIrradiance, v -> photonMapState.precomputeGIrradiance = v),
            1,
            null,
            OptionsGroupPhotonMap::parseBoolInt);
        TypedOption<Integer> pmapDoCausticOpt = new TypedOption<>(
            "-pmap-do-caustic",
            TypedOption.reference(() -> photonMapState.doCausticMap, v -> photonMapState.doCausticMap = v),
            1,
            null,
            OptionsGroupPhotonMap::parseBoolInt);
        TypedOption<Long> pmapCausticPathsOpt = new TypedOption<>(
            "-pmap-caustic-paths",
            TypedOption.reference(() -> photonMapState.cPathsPerIteration, v -> photonMapState.cPathsPerIteration = v),
            1,
            null,
            null);
        TypedOption<Integer> pmapRenderHitsOpt = new TypedOption<>(
            "-pmap-render-hits",
            TypedOption.reference(() -> photonMapState.renderImage, v -> photonMapState.renderImage = v),
            0,
            OptionsGroupPhotonMap::setIntTrue,
            null);
        TypedOption<Integer> pmapReconGPhotonsOpt = new TypedOption<>(
            "-pmap-recon-gphotons",
            TypedOption.reference(() -> photonMapState.reconGPhotons, v -> photonMapState.reconGPhotons = v),
            1,
            null,
            null);
        TypedOption<Integer> pmapReconIPhotonsOpt = new TypedOption<>(
            "-pmap-recon-iphotons",
            TypedOption.reference(() -> photonMapState.reconCPhotons, v -> photonMapState.reconCPhotons = v),
            1,
            null,
            null);
        TypedOption<Integer> pmapReconPhotonsOpt = new TypedOption<>(
            "-pmap-recon-photons",
            TypedOption.reference(() -> photonMapState.reconIPhotons, v -> photonMapState.reconIPhotons = v),
            1,
            null,
            null);
        TypedOption<Integer> pmapBalancingOpt = new TypedOption<>(
            "-pmap-balancing",
            TypedOption.reference(() -> photonMapState.balanceKDTree, v -> photonMapState.balanceKDTree = v),
            1,
            null,
            OptionsGroupPhotonMap::parseBoolInt);
        OptionBase[] photonMapOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(pmapDoGlobalOpt, 9),
            TypedOption.REGISTER_OPTION(pmapGlobalPathsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapGPreirradianceOpt, 11),
            TypedOption.REGISTER_OPTION(pmapDoCausticOpt, 9),
            TypedOption.REGISTER_OPTION(pmapCausticPathsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapRenderHitsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapReconGPhotonsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapReconIPhotonsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapReconPhotonsOpt, 9),
            TypedOption.REGISTER_OPTION(pmapBalancingOpt, 9)
        };
        OptionGroup[] photonMapGroups = new OptionGroup[] {
            new OptionGroup("photonMap", photonMapOptions, 10)
        };
        OptionParser.parse(argc, argv, photonMapGroups, 1);
    }
}
