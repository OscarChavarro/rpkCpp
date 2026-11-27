//= References:                                                             =
//= [WARD1996] Ward, Greg. "The Materials and Geometry Format", version     =
//=     1.0. Lawrence Berkeley Laboratory, February 1996. Section 6.1.2,    =
//=     "Converting between RGB and CIE Colors".                            =
//=     PDF: doc/references/[WARD1996]_*.pdf                                =
//= [COHE1993] Cohen, Michael F. and Wallace, John R. "Radiosity and        =
//=     Realistic Image Synthesis". Academic Press Professional, 1993.      =
//=     Sections 2.3, 9.5.2, 9.6.2 and 9.6.3.                               =
//=     PDF: doc/references/[COHE1993]_*.pdf                                =
//= [ARVO1991] Arvo, James (editor). "Graphics Gems II". Academic Press,    =
//=     1991.                                                               =
//=     PDF: doc/references/[ARVO1991]_*.pdf                                =
//= [MART1991] Martindale, David and Paeth, Alan W. "Television Color       =
//=     Encoding and 'Hot' Broadcast Colors". In [ARVO1991], gem III.6,     =
//=     pp. 147-158.                                                        =
//= [MEYE1991] Meyer, Gary W. "An Inexpensive Method of Setting the         =
//=     Monitor White Point". In [ARVO1991], gem III.7, pp. 159-162.        =
//= [TUMB1993] Tumblin, Jack and Rushmeier, Holly. "Tone Reproduction for   =
//=     Realistic Images". IEEE Computer Graphics and Applications 13(6),   =
//=     November 1993, pp. 42-48.                                           =
//=     PDF: doc/references/[TUMB1993]_*.pdf                                =
//= [FERW1996] Ferwerda, James A., Pattanaik, Sumanta N., Shirley, Peter    =
//=     and Greenberg, Donald P. "A Model of Visual Adaptation for          =
//=     Realistic Image Synthesis". SIGGRAPH '96, 1996, pp. 249-258.        =
//=     PDF: doc/references/[FERW1996]_*.pdf                                =

/**
An RGB color space defined by the CIE xy chromaticities of its three
primaries and of its white point: CIE XYZ <-> RGB conversion and the
achromatic (gray) and photometric (luminance) values of an RGB triple.

Instances are immutable values. Whoever selects the display primaries (the
tone mapping context) owns one and hands it to the code that needs it.

Traceability:
- The conversion code is a modified version of the CIE XYZ <-> RGB code of
  [WARD1996] Sec. 6.1.2: `cieD`, `cieCrD`, `cieCgD` and `cieCbD` are its
  `CIE_D`, `CIE_C_rD`, `CIE_C_gD` and `CIE_C_bD` macros, the gray factors
  are its `CIE_rf`, `CIE_gf` and `CIE_bf`, the two matrices are its
  `xyz2rgbmat` and `rgb2xyzmat`, and the two transforms are its `cie_rgb`
  and `rgb_cie`. Its default primaries are the "nominal CRT primaries" of
  that code; its `NTSC` variant is obtained with the eight-value
  constructor. MGF itself specifies colors as CIE (x, y) chromaticities of
  the 1931 CIE 2 degree standard observer (the `cxy` entity), which is why
  an MGF reader needs this class.
- The CIE XYZ space, the chromaticity coordinates x = X / (X + Y + Z),
  y = Y / (X + Y + Z) and the monitor gamut as a triangle in the
  chromaticity diagram are those of [COHE1993] Sec. 9.6.2 (Eq. 9.21,
  Figs. 9.28 and 9.29). [COHE1993] Sec. 9.6.3 explains why the XYZ <-> RGB
  transform depends on the particular monitor, and so why the primaries are
  parameters of an instance and not global constants.
- The luminance weights of the primaries (the gray factors) are derived
  from the primaries and the white point as in [MART1991]; the white point
  itself is discussed in [MEYE1991].
- Tone mapping operators such as [TUMB1993] and [FERW1996] (see also
  [COHE1993] Sec. 9.5.2) are the consumers of `spectrumLuminance` and
  `getLuminousEfficacy`: they need the real-world luminance of a radiance.
*/
export class Cie {
    /**
    `MAX_EFFICACY'
    `WHITE_EFFICACY'

    Luminous efficacies [lm/W] for conversion between radiometric and
    photometric units (in our case, for conversion between radiance and
    luminance). Normally, the spectral value of a radiometric quantity would
    be scaled by the photopic luminous efficiency function and integrated
    over the visible spectrum; the result multiplied by MAX_EFFICACY would
    give the appropriate photometric value. Without knowing the spectral
    representation, it is generally impossible to perform correct
    radometric->photometric conversion. The "WHITE_EFFICACY" factor is the
    ratio between the luminour powers of the uniform equal-energy white
    spectrum of 1W and its radiant power (which is, surprisingly, 1W). The
    corrected CIE 1988 standard observer curve has been used in this case -
    using the older CIE 1931 curves gives the value of 179 (see the Radiance
    rendering system).

    The integral over the luminous efficiency function is Eq. 2.1 of
    [COHE1993] Sec. 2.3, which gives the peak efficacy as 684 lm/W at 555 nm;
    [WARD1996] uses 683 lm/W as its peak lumens per watt multiplier
    (`C_CMAXV`). The value 183.07 below comes from neither reference.
    */
    private static readonly WHITE_EFFICACY = 183.07;

    private readonly redX: number;
    private readonly redY: number;
    private readonly greenX: number;
    private readonly greenY: number;
    private readonly blueX: number;
    private readonly blueY: number;
    private readonly whiteX: number;
    private readonly whiteY: number;

    private readonly luminousEfficacy: number;
    private redGrayFactor = 0.0;
    private greenGrayFactor = 0.0;
    private blueGrayFactor = 0.0;
    private readonly xyz2RgbMat: number[][] = [
        [0.0, 0.0, 0.0],
        [0.0, 0.0, 0.0],
        [0.0, 0.0, 0.0],
    ];
    private readonly rgb2XyzMat: number[][] = [
        [0.0, 0.0, 0.0],
        [0.0, 0.0, 0.0],
        [0.0, 0.0, 0.0],
    ];

    /**
    The "nominal CRT primaries" and the monitor white point of [WARD1996]
    Sec. 6.1.2 (its non-NTSC branch).
    */
    public constructor();
    /**
    Computes RGB <-> XYZ color transforms based on the given monitor primary
    colors and white point.
    */
    public constructor(xr: number, yr: number, xg: number, yg: number, xb: number, yb: number, xw: number, yw: number);
    public constructor(
        xr = 0.64, // Nominal CRT primaries
        yr = 0.33,
        xg = 0.29,
        yg = 0.6,
        xb = 0.15,
        yb = 0.06,
        xw = 0.3333333333,
        yw = 0.3333333333,
    ) {
        this.redX = xr;
        this.redY = yr;
        this.greenX = xg;
        this.greenY = yg;
        this.blueX = xb;
        this.blueY = yb;
        this.whiteX = xw;
        this.whiteY = yw;
        this.luminousEfficacy = Cie.WHITE_EFFICACY;
        this.computeColorConversionTransforms();
    }

    /**
    `CIE_D` of [WARD1996] Sec. 6.1.2: the determinant of the matrix whose
    columns are the chromaticities of the three primaries.
    */
    private cieD(): number {
        return (
            this.redX * (this.greenY - this.blueY) +
            this.greenX * (this.blueY - this.redY) +
            this.blueX * (this.redY - this.greenY)
        );
    }

    /**
    `CIE_C_rD` of [WARD1996] Sec. 6.1.2. `cieCrD() / cieD()` is the unknown
    J_r of the white point system of [MART1991].
    */
    private cieCrD(): number {
        return (
            (1.0 / this.whiteY) *
            (this.whiteX * (this.greenY - this.blueY) -
                this.whiteY * (this.greenX - this.blueX) +
                this.greenX * this.blueY -
                this.blueX * this.greenY)
        );
    }

    /**
    `CIE_C_gD` of [WARD1996] Sec. 6.1.2 (J_g of [MART1991] times `cieD()`).
    */
    private cieCgD(): number {
        return (
            (1.0 / this.whiteY) *
            (this.whiteX * (this.blueY - this.redY) -
                this.whiteY * (this.blueX - this.redX) -
                this.redX * this.blueY +
                this.blueX * this.redY)
        );
    }

    /**
    `CIE_C_bD` of [WARD1996] Sec. 6.1.2 (J_b of [MART1991] times `cieD()`).
    */
    private cieCbD(): number {
        return (
            (1.0 / this.whiteY) *
            (this.whiteX * (this.redY - this.greenY) -
                this.whiteY * (this.redX - this.greenX) +
                this.redX * this.greenY -
                this.greenX * this.redY)
        );
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
        i: number,
    ): void {
        const row0 = mat[0]!;
        const row1 = mat[1]!;
        const row2 = mat[2]!;
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

    private static colorTransform(col: readonly number[], mat: readonly (readonly number[])[], res: number[]): void {
        const row0 = mat[0]!;
        const row1 = mat[1]!;
        const row2 = mat[2]!;
        const c0 = col[0]!;
        const c1 = col[1]!;
        const c2 = col[2]!;
        res[0] = row0[0]! * c0 + row0[1]! * c1 + row0[2]! * c2;
        res[1] = row1[0]! * c0 + row1[1]! * c1 + row1[2]! * c2;
        res[2] = row2[0]! * c0 + row2[1]! * c1 + row2[2]! * c2;
    }

    /**
    Builds the `xyz2rgbmat` and `rgb2xyzmat` matrices of [WARD1996] Sec. 6.1.2.

    Each column of the RGB to XYZ matrix is the chromaticity (x, y, z) of a
    primary, with z = 1 - x - y ([COHE1993] Eq. 9.21), scaled by the amount
    J = C_D / D of that primary in the white point. [MART1991] finds the J's by
    solving

        | x_r x_g x_b |   | J_r |   | x_w / y_w |
        | y_r y_g y_b | * | J_g | = |     1     |
        | z_r z_g z_b |   | J_b |   | z_w / y_w |

    and gives J_r * y_r, J_g * y_g and J_b * y_b as the relative luminance
    contributions of the primaries to the reference white: the second row of
    the matrix, kept as the gray factors. [MART1991] warns that the familiar
    weights 0.299, 0.587 and 0.114 hold only for the NTSC primaries with the
    Illuminant C white point, which is why they are computed here from the
    chromaticities.
    */
    private computeColorConversionTransforms(): void {
        const d = this.cieD();
        const crD = this.cieCrD();
        const cgD = this.cieCgD();
        const cbD = this.cieCbD();

        // Second row of the RGB to XYZ matrix: the Y (gray) of each primary
        this.redGrayFactor = (this.redY * crD) / d;
        this.greenGrayFactor = (this.greenY * cgD) / d;
        this.blueGrayFactor = (this.blueY * cbD) / d;

        Cie.setColorTransform(
            this.xyz2RgbMat, // XYZ to RGB
            (this.greenY - this.blueY - this.blueX * this.greenY + this.blueY * this.greenX) / crD,
            (this.blueX - this.greenX - this.blueX * this.greenY + this.greenX * this.blueY) / crD,
            (this.greenX * this.blueY - this.blueX * this.greenY) / crD,
            (this.blueY - this.redY - this.blueY * this.redX + this.redY * this.blueX) / cgD,
            (this.redX - this.blueX - this.redX * this.blueY + this.blueX * this.redY) / cgD,
            (this.blueX * this.redY - this.redX * this.blueY) / cgD,
            (this.redY - this.greenY - this.redY * this.greenX + this.greenY * this.redX) / cbD,
            (this.greenX - this.redX - this.greenX * this.redY + this.redX * this.greenY) / cbD,
            (this.redX * this.greenY - this.greenX * this.redY) / cbD,
        );

        Cie.setColorTransform(
            this.rgb2XyzMat, // RGB to XYZ
            (this.redX * crD) / d,
            (this.greenX * cgD) / d,
            (this.blueX * cbD) / d,
            this.redGrayFactor,
            this.greenGrayFactor,
            this.blueGrayFactor,
            ((1.0 - this.redX - this.redY) * crD) / d,
            ((1.0 - this.greenX - this.greenY) * cgD) / d,
            ((1.0 - this.blueX - this.blueY) * cbD) / d,
        );
    }

    public getRedX(): number {
        return this.redX;
    }
    public getRedY(): number {
        return this.redY;
    }
    public getGreenX(): number {
        return this.greenX;
    }
    public getGreenY(): number {
        return this.greenY;
    }
    public getBlueX(): number {
        return this.blueX;
    }
    public getBlueY(): number {
        return this.blueY;
    }
    public getWhiteX(): number {
        return this.whiteX;
    }
    public getWhiteY(): number {
        return this.whiteY;
    }
    public getLuminousEfficacy(): number {
        return this.luminousEfficacy;
    }

    /**
    CIE XYZ -> RGB, `cie_rgb` of [WARD1996] Sec. 6.1.2. Unlike `cie_rgb`, the
    negative components of a color outside the gamut of the primaries (outside
    their triangle in the chromaticity diagram, [COHE1993] Fig. 9.28) are not
    clamped to zero: clamping is left to the caller.
    */
    public transformColorFromXYZ2RGB(xyz: readonly number[], rgb: number[]): void {
        Cie.colorTransform(xyz, this.xyz2RgbMat, rgb);
    }

    /**
    RGB -> CIE XYZ, `rgb_cie` of [WARD1996] Sec. 6.1.2.
    */
    public transformColorFromRGB2XYZ(rgb: readonly number[], xyz: number[]): void {
        Cie.colorTransform(rgb, this.rgb2XyzMat, xyz);
    }

    /**
    Returns an achromatic value representing the spectral quantity: the CIE Y
    of the color, whose matching function is the luminous efficiency function
    ([COHE1993] Sec. 9.6.2). [COHE1993] Sec. 9.6.3 proposes this Y channel as
    the luminance measure for error metrics.
    */
    public spectrumGray(r: number, g: number, b: number): number {
        return this.redGrayFactor * r + this.greenGrayFactor * g + this.blueGrayFactor * b;
    }

    /**
    Returns the luminance, photometric quantity corresponding to the
    radiance of the given spectrum ([COHE1993] Sec. 2.3).
    */
    public spectrumLuminance(r: number, g: number, b: number): number {
        return this.luminousEfficacy * this.spectrumGray(r, g, b);
    }
}
