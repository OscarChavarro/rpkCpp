import { Numeric } from "../../common/linealAlgebra/Numeric";
import { ParseErrorContext } from "./ParseErrorContext";
import { TokenValidationContext } from "./TokenValidationContext";

export class ColorContext {
  public static readonly COLOR_MINIMUM_WAVE_LENGTH = 380;
  public static readonly COLOR_MAXIMUM_WAVE_LENGTH = 780;

  public static readonly COLOR_SPECTRUM_IS_SET_FLAG = 0x01;
  public static readonly COLOR_DEFINED_WITH_SPECTRUM_FLAG = 0x02;
  public static readonly COLOR_XY_IS_SET_FLAG = 0x04;
  public static readonly COLOR_DEFINED_WITH_XY_FLAG = 0x08;
  public static readonly COLOR_EFFICACY_FLAG = 0x10;

  public static readonly NUMBER_OF_SPECTRAL_SAMPLES = 41;
  public static readonly COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE = 10000;
  public static readonly DEFAULT_COLOR_CONTEXT: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_XY_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_EFFICACY_FLAG,
    ColorContext.fill(ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE),
    ColorContext.NUMBER_OF_SPECTRAL_SAMPLES * ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
    1.0 / 3.0,
    1.0 / 3.0,
    178.006,
  );

  // W-m^2
  private static readonly C1 = 3.741832e-16;
  // m-K
  private static readonly C2 = 1.4388e-2;

  // Derived CIE 1931 primaries (imaginary)
  private static readonly cie_xp: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG,
    [
      -174, -198, -195, -197, -202, -213, -235, -272, -333,
      -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
      -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
      7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
      8336, 8336, 8336, 8336, 8336, 8336,
    ],
    127424,
    1.0,
    0.0,
    0.0,
  );

  private static readonly cie_yp: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG,
    [
      -451, -431, -431, -430, -427, -417, -399, -366, -312,
      -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
      3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
      -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
      -4066, -4066, -4066, -4066, -4066, -4066,
    ],
    -23035,
    0.0,
    1.0,
    0.0,
  );

  private static readonly cie_zp: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG,
    [
      4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
      4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
      -904, -918, -898, -884, -869, -863, -855, -855, -851,
      -848, -847, -846, -846, -846, -845, -846, -843, -845,
      -845, -845, -845, -845, -845,
    ],
    36057,
    0.0,
    0.0,
    0.0,
  );

  // CIE 1931 standard observer curves
  private static readonly cie_xf: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG
      | ColorContext.COLOR_EFFICACY_FLAG,
    [
      14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
      320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
      10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
      114, 58, 29, 14, 7, 3, 2, 1, 0,
    ],
    106836,
    0.467,
    0.368,
    362.23,
  );

  private static readonly cie_yf: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG
      | ColorContext.COLOR_EFFICACY_FLAG,
    [
      0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
      5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
      5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
      5, 2, 1, 1, 0, 0,
    ],
    106856,
    0.398,
    0.542,
    493.525,
  );

  private static readonly cie_zf: ColorContext = ColorContext.createContext(
    1,
    ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG
      | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG
      | ColorContext.COLOR_XY_IS_SET_FLAG
      | ColorContext.COLOR_EFFICACY_FLAG,
    [
      65, 201, 679, 2074, 6456, 13856, 17471, 17721, 16692,
      12876, 8130, 4652, 2720, 1582, 782, 422, 203, 87, 39, 21, 17,
      11, 8, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ],
    106770,
    0.147,
    0.077,
    54.363,
  );

  public clock: number;
  public flags: number;
  public straightSamples: number[];
  public spectralStraightSum: number;
  public cx: number;
  public cy: number;
  public eff: number;

  public constructor() {
    this.clock = 0;
    this.flags = 0;
    this.straightSamples = new Array<number>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES).fill(0);
    this.spectralStraightSum = 0;
    this.cx = 0.0;
    this.cy = 0.0;
    this.eff = 0.0;
  }

  public copy(source: ColorContext | null): void {
    if (source === null) {
      return;
    }
    this.clock = source.clock;
    this.flags = source.flags;
    for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
      this.straightSamples[i] = source.straightSamples[i];
    }
    this.spectralStraightSum = source.spectralStraightSum;
    this.cx = source.cx;
    this.cy = source.cy;
    this.eff = source.eff;
  }

  /**
  Convert a spectrum.
  */
  public setSpectrum(wlMinimum: number, wlMaximum: number, ac: number, av: string[] | null): number {
    let scale: number;
    const va = new Array<number>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 1).fill(0.0);
    let pos: number;
    let n: number;
    let imax: number;
    let wl: number;
    let wl0: number;
    let wlStep: number;
    let boxPos: number;
    let boxStep: number;
    let argumentStartIndex = 0;

    if (av === null || ac < 2 || av.length < ac) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    if (
      wlMaximum <= ColorContext.COLOR_MINIMUM_WAVE_LENGTH
      || wlMaximum <= wlMinimum
      || wlMinimum >= ColorContext.COLOR_MAXIMUM_WAVE_LENGTH
    ) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wlStep = (wlMaximum - wlMinimum) / (ac - 1);
    while (wlMinimum < ColorContext.COLOR_MINIMUM_WAVE_LENGTH) {
      wlMinimum += wlStep;
      ac--;
      argumentStartIndex++;
    }
    while (wlMaximum > ColorContext.COLOR_MAXIMUM_WAVE_LENGTH) {
      wlMaximum -= wlStep;
      ac--;
    }

    imax = ac;
    boxPos = 0.0;
    boxStep = 1.0;
    if (wlStep < ColorContext.colorWaveLengthDeltaI()) {
      imax = Math.round((wlMaximum - wlMinimum) / ColorContext.colorWaveLengthDeltaI() + (1 - Numeric.EPSILON));
      boxPos = (wlMinimum - ColorContext.COLOR_MINIMUM_WAVE_LENGTH) / ColorContext.colorWaveLengthDeltaI();
      boxStep = wlStep / ColorContext.colorWaveLengthDeltaI();
      wlStep = ColorContext.colorWaveLengthDeltaI();
    }

    scale = 0.0;
    pos = 0;
    for (let i = 0; i < imax; i++) {
      va[i] = 0.0;
      n = 0;
      while (boxPos < i + 0.5 && pos < ac) {
        const value = av[argumentStartIndex + pos];
        if (!TokenValidationContext.isFloat(value)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        va[i] += Number.parseFloat(value);
        pos++;
        n++;
        boxPos += boxStep;
      }
      if (n > 1) {
        va[i] /= n;
      }
      if (va[i] > scale) {
        scale = va[i];
      }
      else if (va[i] < -scale) {
        scale = -va[i];
      }
    }

    if (scale <= Numeric.EPSILON) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    scale = ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
    this.spectralStraightSum = 0;
    wl0 = wlMinimum;
    pos = 0;
    for (
      let i = 0, waveLength = ColorContext.COLOR_MINIMUM_WAVE_LENGTH;
      i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES;
      i++, waveLength += ColorContext.colorWaveLengthDeltaI()
    ) {
      wl = waveLength;
      if (wl < wlMinimum || wl > wlMaximum) {
        this.straightSamples[i] = 0;
      }
      else {
        while (wl0 + wlStep < wl + Numeric.EPSILON) {
          wl0 += wlStep;
          pos++;
        }
        if (wl + Numeric.EPSILON >= wl0 && wl - Numeric.EPSILON <= wl0) {
          this.straightSamples[i] = Math.round(scale * va[pos] + 0.5);
        }
        else {
          const pos1 = Math.min(pos + 1, va.length - 1);
          this.straightSamples[i] = Math.round(
            0.5 + scale / wlStep * (va[pos] * (wl0 + wlStep - wl) + va[pos1] * (wl - wl0)),
          );
        }
        this.spectralStraightSum += this.straightSamples[i];
      }
    }
    this.flags = ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG;
    this.clock++;
    return ParseErrorContext.MGF_OK;
  }

  /**
  Set black body spectrum.
  */
  public setBlackBodyTemperature(tk: number): number {
    let sf: number;
    let wl: number;

    if (tk < 1000) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = ColorContext.bBlm(tk);
    if (wl < ColorContext.COLOR_MINIMUM_WAVE_LENGTH * 1e-9) {
      wl = ColorContext.COLOR_MINIMUM_WAVE_LENGTH * 1e-9;
    }
    else if (wl > ColorContext.COLOR_MAXIMUM_WAVE_LENGTH * 1e-9) {
      wl = ColorContext.COLOR_MAXIMUM_WAVE_LENGTH * 1e-9;
    }
    sf = ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / ColorContext.bBsp(wl, tk);
    this.spectralStraightSum = 0;
    for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
      wl = (ColorContext.COLOR_MINIMUM_WAVE_LENGTH + i * ColorContext.colorWaveLengthDeltaI()) * 1e-9;
      this.straightSamples[i] = Math.round(sf * ColorContext.bBsp(wl, tk) + 0.5);
      this.spectralStraightSum += this.straightSamples[i];
    }
    this.flags = ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG;
    this.clock++;
    return ParseErrorContext.MGF_OK;
  }

  /**
  Convert color representations.
  */
  public fixColorRepresentation(fl: number): void {
    let x: number;
    let y: number;
    let z: number;

    fl &= ~this.flags;
    if (fl === 0) {
      return;
    }
    if ((this.flags & (ColorContext.COLOR_XY_IS_SET_FLAG | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG)) === 0) {
      this.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    }

    if ((fl & ColorContext.COLOR_XY_IS_SET_FLAG) !== 0) {
      x = 0.0;
      y = 0.0;
      z = 0.0;
      for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
        x += ColorContext.cie_xf.straightSamples[i] * this.straightSamples[i];
        y += ColorContext.cie_yf.straightSamples[i] * this.straightSamples[i];
        z += ColorContext.cie_zf.straightSamples[i] * this.straightSamples[i];
      }
      x /= ColorContext.cie_xf.spectralStraightSum;
      y /= ColorContext.cie_yf.spectralStraightSum;
      z /= ColorContext.cie_zf.spectralStraightSum;
      z += x + y;
      this.cx = x / z;
      this.cy = y / z;
      this.flags |= ColorContext.COLOR_XY_IS_SET_FLAG;
    }
    else if ((fl & ColorContext.COLOR_SPECTRUM_IS_SET_FLAG) !== 0) {
      x = this.cx;
      y = this.cy;
      z = 1.0 - x - y;
      this.spectralStraightSum = 0;
      for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
        this.straightSamples[i] = Math.round(
          x * ColorContext.cie_xp.straightSamples[i]
            + y * ColorContext.cie_yp.straightSamples[i]
            + z * ColorContext.cie_zp.straightSamples[i]
            + 0.5,
        );
        if (this.straightSamples[i] < 0) {
          this.straightSamples[i] = 0;
        }
        else {
          this.spectralStraightSum += this.straightSamples[i];
        }
      }
      this.flags |= ColorContext.COLOR_SPECTRUM_IS_SET_FLAG;
    }

    if ((fl & ColorContext.COLOR_EFFICACY_FLAG) !== 0) {
      if ((this.flags & ColorContext.COLOR_SPECTRUM_IS_SET_FLAG) !== 0) {
        y = 0.0;
        for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
          y += ColorContext.cie_yf.straightSamples[i] * this.straightSamples[i];
        }
        this.eff = ColorContext.colorPeakLumensPerWatt() * y / this.spectralStraightSum;
      }
      else {
        this.eff = this.cx * ColorContext.cie_xf.eff
          + this.cy * ColorContext.cie_yf.eff
          + (1.0 - this.cx - this.cy) * ColorContext.cie_zf.eff;
      }
      this.flags |= ColorContext.COLOR_EFFICACY_FLAG;
    }
  }

  /**
  Mix two colors according to weights given.
  */
  public mixColors(w1: number, c1: ColorContext, w2: number, c2: ColorContext): void {
    let scale: number;
    const cMix = new Array<number>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES).fill(0.0);

    if (((c1.flags | c2.flags) & ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG) !== 0) {
      c1.fixColorRepresentation(ColorContext.COLOR_SPECTRUM_IS_SET_FLAG | ColorContext.COLOR_EFFICACY_FLAG);
      c2.fixColorRepresentation(ColorContext.COLOR_SPECTRUM_IS_SET_FLAG | ColorContext.COLOR_EFFICACY_FLAG);
      w1 /= c1.eff * c1.spectralStraightSum;
      w2 /= c2.eff * c2.spectralStraightSum;
      scale = 0.0;
      for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
        cMix[i] = w1 * c1.straightSamples[i] + w2 * c2.straightSamples[i];
        if (cMix[i] > scale) {
          scale = cMix[i];
        }
      }
      scale = ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
      this.spectralStraightSum = 0;
      for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
        this.straightSamples[i] = Math.round(scale * cMix[i] + 0.5);
        this.spectralStraightSum += this.straightSamples[i];
      }
      this.flags = ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG | ColorContext.COLOR_SPECTRUM_IS_SET_FLAG;
    }
    else {
      c1.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
      c2.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
      scale = w1 / c1.cy + w2 / c2.cy;
      if (scale === 0.0) {
        return;
      }
      scale = 1.0 / scale;
      this.cx = (c1.cx * w1 / c1.cy + c2.cx * w2 / c2.cy) * scale;
      this.cy = (w1 + w2) * scale;
      this.flags = ColorContext.COLOR_DEFINED_WITH_XY_FLAG | ColorContext.COLOR_XY_IS_SET_FLAG;
    }
  }

  private static createContext(
    inClock: number,
    inFlags: number,
    samples: number[],
    sum: number,
    inCx: number,
    inCy: number,
    inEff: number,
  ): ColorContext {
    const colorContext = new ColorContext();
    colorContext.clock = inClock;
    colorContext.flags = inFlags;
    for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
      colorContext.straightSamples[i] = samples[i];
    }
    colorContext.spectralStraightSum = sum;
    colorContext.cx = inCx;
    colorContext.cy = inCy;
    colorContext.eff = inEff;
    return colorContext;
  }

  private static fill(value: number): number[] {
    const values = new Array<number>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES).fill(0);
    for (let i = 0; i < values.length; i++) {
      values[i] = value;
    }
    return values;
  }

  private static colorWaveLengthDeltaI(): number {
    return (ColorContext.COLOR_MAXIMUM_WAVE_LENGTH - ColorContext.COLOR_MINIMUM_WAVE_LENGTH)
      / (ColorContext.NUMBER_OF_SPECTRAL_SAMPLES - 1);
  }

  private static colorPeakLumensPerWatt(): number {
    return 683.0 / ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE;
  }

  private static bBlm(t: number): number {
    return ColorContext.C2 / 5.0 / t;
  }

  private static bBsp(l: number, t: number): number {
    return ColorContext.C1 / (l * l * l * l * l * (Math.exp(ColorContext.C2 / (t * l)) - 1.0));
  }
}
