export class BatchOptions {
  public exportBinary: boolean;
  public binaryOutputFilename: string;
  public importBinary: boolean;
  public binaryInputFilename: string;
  public iterations: number;
  public radianceImageFileNameFormat: string;
  public radianceModelFileNameFormat: string;
  public saveModulo: number; // Every n-th iteration the image will be saved (the model at every iteration)
  public saveLastOnly: number; // Image and model saved only after the last iteration (overrides saveModulo)
  public raytracingImageFileName: string;
  public timings: number;

  public constructor() {
    this.exportBinary = false;
    this.binaryOutputFilename = "";
    this.importBinary = false;
    this.binaryInputFilename = "";
    this.iterations = 1;
    this.radianceImageFileNameFormat = "";
    this.radianceModelFileNameFormat = "";
    this.saveModulo = 10;
    this.saveLastOnly = 0;
    this.raytracingImageFileName = "";
    this.timings = 0;
  }
}
