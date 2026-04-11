/**
Stores luminance-area pairs for median area-weighted luminance
selection.
*/
export class LuminanceArea {
  public luminance: number;
  public area: number;

  public constructor() {
    this.luminance = 0.0;
    this.area = 0.0;
  }
}
