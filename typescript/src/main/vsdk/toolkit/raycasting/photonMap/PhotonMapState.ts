import { PhotonMapDCAcceptPDFType } from "./PhotonMapDCAcceptPDFType";
import { PhotonMapDensityControlOption } from "./PhotonMapDensityControlOption";
import { PhotonMapImportanceOptions } from "./PhotonMapImportanceOptions";
import { RadiosityReturnOption } from "./RadiosityReturnOption";

export class PhotonMapState {
  public static readonly MAXIMUM_RECON_PHOTONS = 400;

  public doGlobalMap: number;
  public gPathsPerIteration: number;
  public precomputeGIrradiance: number;
  public doCausticMap: number;
  public cPathsPerIteration: number;
  public renderImage: number;
  public reconGPhotons: number;
  public reconCPhotons: number;
  public reconIPhotons: number;
  public distribPhotons: number;
  public balanceKDTree: number;
  public usePhotonMapSampler: number;
  public densityControl: PhotonMapDensityControlOption;
  public importanceOption: number;
  public acceptPdfType: PhotonMapDCAcceptPDFType;
  public constantRD: number;
  public minimumImpRD: number;
  public doImportanceMap: number;
  public iPathsPerIteration: number;
  public cImpScale: number;
  public gImpScale: number;
  public gThreshold: number;
  public falseColMax: number;
  public falseColLog: number;
  public falseColMono: number;
  public radianceReturn: RadiosityReturnOption;
  public minimumLightPathDepth: number;
  public maximumLightPathDepth: number;
  public iterationNumber: number;
  public gIterationNumber: number;
  public cIterationNumber: number;
  public i_iteration_nr: number;
  public totalCPaths: number;
  public totalGPaths: number;
  public totalIPaths: number;
  public runStopNumber: number;
  public cpuSecs: number;
  public lastClock: number;

  public constructor() {
    this.doGlobalMap = 0;
    this.gPathsPerIteration = 0;
    this.precomputeGIrradiance = 0;
    this.doCausticMap = 0;
    this.cPathsPerIteration = 0;
    this.renderImage = 0;
    this.reconGPhotons = 0;
    this.reconCPhotons = 0;
    this.reconIPhotons = 0;
    this.distribPhotons = 0;
    this.balanceKDTree = 0;
    this.usePhotonMapSampler = 0;
    this.densityControl = PhotonMapDensityControlOption.NO_DENSITY_CONTROL;
    this.importanceOption = PhotonMapImportanceOptions.USE_IMPORTANCE;
    this.acceptPdfType = PhotonMapDCAcceptPDFType.STEP;
    this.constantRD = 0.0;
    this.minimumImpRD = 0.0;
    this.doImportanceMap = 0;
    this.iPathsPerIteration = 0;
    this.cImpScale = 0.0;
    this.gImpScale = 0.0;
    this.gThreshold = 0.0;
    this.falseColMax = 0.0;
    this.falseColLog = 0;
    this.falseColMono = 0;
    this.radianceReturn = RadiosityReturnOption.GLOBAL_RADIANCE;
    this.minimumLightPathDepth = 0;
    this.maximumLightPathDepth = 0;
    this.iterationNumber = 0;
    this.gIterationNumber = 0;
    this.cIterationNumber = 0;
    this.i_iteration_nr = 0;
    this.totalCPaths = 0;
    this.totalGPaths = 0;
    this.totalIPaths = 0;
    this.runStopNumber = 0;
    this.cpuSecs = 0.0;
    this.lastClock = 0;

    this.setDefaults();
  }

  public setDefaults(): void {
    this.doCausticMap = 1;
    this.cPathsPerIteration = 20000;

    this.doGlobalMap = 1;
    this.gPathsPerIteration = 10000;
    this.precomputeGIrradiance = 1;

    this.renderImage = 0;

    this.reconGPhotons = 80;
    this.reconCPhotons = 80;
    this.reconIPhotons = 200;
    this.distribPhotons = 20;

    this.balanceKDTree = 1;
    this.usePhotonMapSampler = 0;

    this.densityControl = PhotonMapDensityControlOption.NO_DENSITY_CONTROL;
    this.acceptPdfType = PhotonMapDCAcceptPDFType.STEP;

    this.constantRD = 10000;

    this.minimumImpRD = 1;
    this.doImportanceMap = 1;
    this.iPathsPerIteration = 10000;

    this.importanceOption = PhotonMapImportanceOptions.USE_IMPORTANCE;

    this.cImpScale = 25.0;
    this.gImpScale = 1.0;

    this.gThreshold = 1000;

    this.falseColMax = 10000;
    this.falseColLog = 0;
    this.falseColMono = 0;

    this.radianceReturn = RadiosityReturnOption.GLOBAL_RADIANCE;

    this.minimumLightPathDepth = 0;
    this.maximumLightPathDepth = 7;
  }
}

