export class BatchOptions {
  public exportBinary: boolean;
  public binaryOutputFilename: string;
  public importBinary: boolean;
  public binaryInputFilename: string;
  public iterations: number;
  public radianceImageFileNameFormat: string;
  public radianceModelFileNameFormat: string;
  public saveModulo: number;
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
    this.raytracingImageFileName = "";
    this.timings = 0;
  }
}
