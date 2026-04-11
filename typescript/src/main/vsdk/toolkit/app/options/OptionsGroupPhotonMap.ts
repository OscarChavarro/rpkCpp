import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { PhotonMapState } from "../../raycasting/photonMap/PhotonMapState";
import { OptionTextUtils } from "./OptionTextUtils";

export class OptionsGroupPhotonMap {
  private constructor() {
  }

  private static parseBoolInt(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<number>
  ): boolean {
    if (argc < 1 || argv === null || argv[0] === null) {
      return false;
    }
    const parsed = new OptionTextUtils.TypedIntValue(value.value);
    if (!OptionTextUtils.parseBoolInt(argv[0], parsed)) {
      return false;
    }
    value.value = parsed.value;
    return true;
  }

  private static setIntTrue(value: TypedOption.MutableValue<number>): void {
    value.value = 1;
  }

  public static parse(argc: number[], argv: string[], photonMapState: PhotonMapState): void {
    const pmapDoGlobalOpt = new TypedOption<number>(
      "-pmap-do-global",
      TypedOption.reference(() => photonMapState.doGlobalMap, (v) => {
        photonMapState.doGlobalMap = v;
      }),
      1,
      null,
      OptionsGroupPhotonMap.parseBoolInt
    );
    const pmapGlobalPathsOpt = new TypedOption<number>(
      "-pmap-global-paths",
      TypedOption.reference(() => photonMapState.gPathsPerIteration, (v) => {
        photonMapState.gPathsPerIteration = v;
      }),
      1,
      null,
      null
    );
    const pmapGPreirradianceOpt = new TypedOption<number>(
      "-pmap-g-preirradiance",
      TypedOption.reference(() => photonMapState.precomputeGIrradiance, (v) => {
        photonMapState.precomputeGIrradiance = v;
      }),
      1,
      null,
      OptionsGroupPhotonMap.parseBoolInt
    );
    const pmapDoCausticOpt = new TypedOption<number>(
      "-pmap-do-caustic",
      TypedOption.reference(() => photonMapState.doCausticMap, (v) => {
        photonMapState.doCausticMap = v;
      }),
      1,
      null,
      OptionsGroupPhotonMap.parseBoolInt
    );
    const pmapCausticPathsOpt = new TypedOption<number>(
      "-pmap-caustic-paths",
      TypedOption.reference(() => photonMapState.cPathsPerIteration, (v) => {
        photonMapState.cPathsPerIteration = v;
      }),
      1,
      null,
      null
    );
    const pmapRenderHitsOpt = new TypedOption<number>(
      "-pmap-render-hits",
      TypedOption.reference(() => photonMapState.renderImage, (v) => {
        photonMapState.renderImage = v;
      }),
      0,
      OptionsGroupPhotonMap.setIntTrue,
      null
    );
    const pmapReconGPhotonsOpt = new TypedOption<number>(
      "-pmap-recon-gphotons",
      TypedOption.reference(() => photonMapState.reconGPhotons, (v) => {
        photonMapState.reconGPhotons = v;
      }),
      1,
      null,
      null
    );
    const pmapReconIPhotonsOpt = new TypedOption<number>(
      "-pmap-recon-iphotons",
      TypedOption.reference(() => photonMapState.reconCPhotons, (v) => {
        photonMapState.reconCPhotons = v;
      }),
      1,
      null,
      null
    );
    const pmapReconPhotonsOpt = new TypedOption<number>(
      "-pmap-recon-photons",
      TypedOption.reference(() => photonMapState.reconIPhotons, (v) => {
        photonMapState.reconIPhotons = v;
      }),
      1,
      null,
      null
    );
    const pmapBalancingOpt = new TypedOption<number>(
      "-pmap-balancing",
      TypedOption.reference(() => photonMapState.balanceKDTree, (v) => {
        photonMapState.balanceKDTree = v;
      }),
      1,
      null,
      OptionsGroupPhotonMap.parseBoolInt
    );
    const photonMapOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(pmapDoGlobalOpt, 9),
      TypedOption.REGISTER_OPTION(pmapGlobalPathsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapGPreirradianceOpt, 11),
      TypedOption.REGISTER_OPTION(pmapDoCausticOpt, 9),
      TypedOption.REGISTER_OPTION(pmapCausticPathsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapRenderHitsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapReconGPhotonsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapReconIPhotonsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapReconPhotonsOpt, 9),
      TypedOption.REGISTER_OPTION(pmapBalancingOpt, 9),
    ];
    const photonMapGroups = [
      new OptionGroup("photonMap", photonMapOptions, 10),
    ];
    OptionParser.parse(argc, argv, photonMapGroups, 1);
  }
}
