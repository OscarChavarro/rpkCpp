export class Cie {
  private static cieXR = 0.640;
  private static cieYR = 0.330;
  private static cieXG = 0.290;
  private static cieYG = 0.600;
  private static cieXB = 0.150;
  private static cieYB = 0.060;
  private static cieXW = 0.3333333333;
  private static cieYW = 0.3333333333;

  public static readonly WHITE_EFFICACY = 183.07;

  private static luminousEfficacy = Cie.WHITE_EFFICACY;
  private static readonly xyz2RgbMat = [
    [0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0]
  ];
  private static readonly rgb2XyzMat = [
    [0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0]
  ];

  static {
    Cie.computeColorConversionTransforms(
      Cie.cieXR,
      Cie.cieYR,
      Cie.cieXG,
      Cie.cieYG,
      Cie.cieXB,
      Cie.cieYB,
      Cie.cieXW,
      Cie.cieYW
    );
  }

  private constructor() {
  }

  private static cieD(): number {
    return Cie.cieXR * (Cie.cieYG - Cie.cieYB) +
      Cie.cieXG * (Cie.cieYB - Cie.cieYR) +
      Cie.cieXB * (Cie.cieYR - Cie.cieYG);
  }

  private static cieCrD(): number {
    return (1.0 / Cie.cieYW) *
      (Cie.cieXW * (Cie.cieYG - Cie.cieYB) -
        Cie.cieYW * (Cie.cieXG - Cie.cieXB) +
        Cie.cieXG * Cie.cieYB - Cie.cieXB * Cie.cieYG);
  }

  private static cieCgD(): number {
    return (1.0 / Cie.cieYW) *
      (Cie.cieXW * (Cie.cieYB - Cie.cieYR) -
        Cie.cieYW * (Cie.cieXB - Cie.cieXR) -
        Cie.cieXR * Cie.cieYB + Cie.cieXB * Cie.cieYR);
  }

  private static cieCbD(): number {
    return (1.0 / Cie.cieYW) *
      (Cie.cieXW * (Cie.cieYR - Cie.cieYG) -
        Cie.cieYW * (Cie.cieXR - Cie.cieXG) +
        Cie.cieXR * Cie.cieYG - Cie.cieXG * Cie.cieYR);
  }

  private static cieRf(): number {
    return Cie.cieYR * Cie.cieCrD() / Cie.cieD();
  }

  private static cieGf(): number {
    return Cie.cieYG * Cie.cieCgD() / Cie.cieD();
  }

  private static cieBf(): number {
    return Cie.cieYB * Cie.cieCbD() / Cie.cieD();
  }

  private static gray(r: number, g: number, b: number): number {
    return Cie.cieRf() * r + Cie.cieGf() * g + Cie.cieBf() * b;
  }

  private static luminance(r: number, g: number, b: number): number {
    return Cie.luminousEfficacy * Cie.gray(r, g, b);
  }

  private static setColorTransform(
    mat: number[][],
    a: number,
    b: number,
    c: number,
    d: number,
    e: number,
    f: number,
    g: number,
    h: number,
    i: number
  ): void {
    const row0 = mat[0];
    const row1 = mat[1];
    const row2 = mat[2];
    if (row0 === undefined || row1 === undefined || row2 === undefined) {
      return;
    }
    row0[0] = a;
    row0[1] = b;
    row0[2] = c;
    row1[0] = d;
    row1[1] = e;
    row1[2] = f;
    row2[0] = g;
    row2[1] = h;
    row2[2] = i;
  }

  private static colorTransform(col: number[], mat: number[][], res: number[]): void {
    const row0 = mat[0];
    const row1 = mat[1];
    const row2 = mat[2];
    const c0 = col[0];
    const c1 = col[1];
    const c2 = col[2];
    if (
      row0 === undefined || row1 === undefined || row2 === undefined ||
      c0 === undefined || c1 === undefined || c2 === undefined
    ) {
      return;
    }
    res[0] = row0[0]! * c0 + row0[1]! * c1 + row0[2]! * c2;
    res[1] = row1[0]! * c0 + row1[1]! * c1 + row1[2]! * c2;
    res[2] = row2[0]! * c0 + row2[1]! * c1 + row2[2]! * c2;
  }

  public static transformColorFromXYZ2RGB(xyz: number[], rgb: number[]): void {
    Cie.colorTransform(xyz, Cie.xyz2RgbMat, rgb);
  }

  public static clipGamut(rgb: number[]): boolean {
    let desaturated = false;
    for (let i = 0; i < 3; i++) {
      const value = rgb[i];
      if (value !== undefined && value < 0.0) {
        rgb[i] = 0.0;
        desaturated = true;
      }
    }
    return desaturated;
  }

  public static computeColorConversionTransforms(
    xr: number,
    yr: number,
    xg: number,
    yg: number,
    xb: number,
    yb: number,
    xw: number,
    yw: number
  ): void {
    Cie.cieXR = xr;
    Cie.cieYR = yr;
    Cie.cieXG = xg;
    Cie.cieYG = yg;
    Cie.cieXB = xb;
    Cie.cieYB = yb;
    Cie.cieXW = xw;
    Cie.cieYW = yw;

    Cie.setColorTransform(
      Cie.xyz2RgbMat,
      (Cie.cieYG - Cie.cieYB - Cie.cieXB * Cie.cieYG + Cie.cieYB * Cie.cieXG) / Cie.cieCrD(),
      (Cie.cieXB - Cie.cieXG - Cie.cieXB * Cie.cieYG + Cie.cieXG * Cie.cieYB) / Cie.cieCrD(),
      (Cie.cieXG * Cie.cieYB - Cie.cieXB * Cie.cieYG) / Cie.cieCrD(),
      (Cie.cieYB - Cie.cieYR - Cie.cieYB * Cie.cieXR + Cie.cieYR * Cie.cieXB) / Cie.cieCgD(),
      (Cie.cieXR - Cie.cieXB - Cie.cieXR * Cie.cieYB + Cie.cieXB * Cie.cieYR) / Cie.cieCgD(),
      (Cie.cieXB * Cie.cieYR - Cie.cieXR * Cie.cieYB) / Cie.cieCgD(),
      (Cie.cieYR - Cie.cieYG - Cie.cieYR * Cie.cieXG + Cie.cieYG * Cie.cieXR) / Cie.cieCbD(),
      (Cie.cieXG - Cie.cieXR - Cie.cieXG * Cie.cieYR + Cie.cieXR * Cie.cieYG) / Cie.cieCbD(),
      (Cie.cieXR * Cie.cieYG - Cie.cieXG * Cie.cieYR) / Cie.cieCbD()
    );

    Cie.setColorTransform(
      Cie.rgb2XyzMat,
      (Cie.cieXR * Cie.cieCrD()) / Cie.cieD(),
      (Cie.cieXG * Cie.cieCgD()) / Cie.cieD(),
      (Cie.cieXB * Cie.cieCbD()) / Cie.cieD(),
      (Cie.cieYR * Cie.cieCrD()) / Cie.cieD(),
      (Cie.cieYG * Cie.cieCgD()) / Cie.cieD(),
      (Cie.cieYB * Cie.cieCbD()) / Cie.cieD(),
      ((1.0 - Cie.cieXR - Cie.cieYR) * Cie.cieCrD()) / Cie.cieD(),
      ((1.0 - Cie.cieXG - Cie.cieYG) * Cie.cieCgD()) / Cie.cieD(),
      ((1.0 - Cie.cieXB - Cie.cieYB) * Cie.cieCbD()) / Cie.cieD()
    );
  }

  public static getLuminousEfficacy(): number {
    return Cie.luminousEfficacy;
  }

  public static spectrumGray(r: number, g: number, b: number): number {
    return Cie.gray(r, g, b);
  }

  public static spectrumLuminance(r: number, g: number, b: number): number {
    return Cie.luminance(r, g, b);
  }
}
