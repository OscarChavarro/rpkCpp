package vsdk.toolkit.io.context;

import vsdk.toolkit.common.linealAlgebra.Numeric;

public class ColorContext {
    public static final int COLOR_MINIMUM_WAVE_LENGTH = 380;
    public static final int COLOR_MAXIMUM_WAVE_LENGTH = 780;

    public static final int COLOR_SPECTRUM_IS_SET_FLAG = 0x01;
    public static final int COLOR_DEFINED_WITH_SPECTRUM_FLAG = 0x02;
    public static final int COLOR_XY_IS_SET_FLAG = 0x04;
    public static final int COLOR_DEFINED_WITH_XY_FLAG = 0x08;
    public static final int COLOR_EFFICACY_FLAG = 0x10;

    public static final int NUMBER_OF_SPECTRAL_SAMPLES = 41; // Number of spectral samples
    public static final int COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE = 10000;
    public static final ColorContext DEFAULT_COLOR_CONTEXT;

    // W-m^2
    private static final double C1 = 3.741832e-16;
    // m-K
    private static final double C2 = 1.4388e-2;
    private static ColorContext cie_xp;
    private static ColorContext cie_yp;
    private static ColorContext cie_zp;
    private static ColorContext cie_xf;
    private static ColorContext cie_yf;
    private static ColorContext cie_zf;

    public int clock; // Incremented each change
    public short flags; // What's been set
    public short[] straightSamples; // Spectral samples, min wl to max
    public long spectralStraightSum; // Straight sum of spectral values
    public float cx; // Chromaticity X value
    public float cy; // Chromaticity Y value
    public float eff; // Efficacy (lumens / watt)

    static {
        DEFAULT_COLOR_CONTEXT = createContext(
            1,
            COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
            fill(COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE),
            (long)NUMBER_OF_SPECTRAL_SAMPLES * COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
            (float)(1.0 / 3.0),
            (float)(1.0 / 3.0),
            178.006f);

        // Derived CIE 1931 Primaries (imaginary)
        cie_xp = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
            new int[] {
                -174, -198, -195, -197, -202, -213, -235, -272, -333,
                -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
                -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
                7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
                8336, 8336, 8336, 8336, 8336, 8336
            },
            127424L,
            1.0f,
            0.0f,
            0.0f);

        cie_yp = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
            new int[] {
                -451, -431, -431, -430, -427, -417, -399, -366, -312,
                -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
                3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
                -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
                -4066, -4066, -4066, -4066, -4066, -4066
            },
            -23035L,
            0.0f,
            1.0f,
            0.0f);

        cie_zp = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
            new int[] {
                4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
                4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
                -904, -918, -898, -884, -869, -863, -855, -855, -851,
                -848, -847, -846, -846, -846, -845, -846, -843, -845,
                -845, -845, -845, -845, -845
            },
            36057L,
            0.0f,
            0.0f,
            0.0f);

        // CIE 1931 Standard Observer curves
        cie_xf = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
            new int[] {
                14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
                320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
                10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
                114, 58, 29, 14, 7, 3, 2, 1, 0
            },
            106836L,
            0.467f,
            0.368f,
            362.230f);

        cie_yf = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
            new int[] {
                0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
                5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
                5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
                5, 2, 1, 1, 0, 0
            },
            106856L,
            0.398f,
            0.542f,
            493.525f);

        cie_zf = createContext(
            1,
            COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
            new int[] {
                65, 201, 679, 2074, 6456, 13856, 17471, 17721, 16692,
                12876, 8130, 4652, 2720, 1582, 782, 422, 203, 87, 39, 21, 17,
                11, 8, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
            106770L,
            0.147f,
            0.077f,
            54.363f);
    }

    public ColorContext() {
        clock = 0;
        flags = 0;
        straightSamples = new short[NUMBER_OF_SPECTRAL_SAMPLES];
        spectralStraightSum = 0;
        cx = 0.0f;
        cy = 0.0f;
        eff = 0.0f;
    }

    public void copy(ColorContext source) {
        if (source == null) {
            return;
        }
        clock = source.clock;
        flags = source.flags;
        System.arraycopy(source.straightSamples, 0, straightSamples, 0, NUMBER_OF_SPECTRAL_SAMPLES);
        spectralStraightSum = source.spectralStraightSum;
        cx = source.cx;
        cy = source.cy;
        eff = source.eff;
    }

    private static ColorContext createContext(int inClock, int inFlags, int[] samples, long sum, float inCx, float inCy, float inEff) {
        ColorContext colorContext = new ColorContext();
        colorContext.clock = inClock;
        colorContext.flags = (short)inFlags;
        for (int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++) {
            colorContext.straightSamples[i] = (short)samples[i];
        }
        colorContext.spectralStraightSum = sum;
        colorContext.cx = inCx;
        colorContext.cy = inCy;
        colorContext.eff = inEff;
        return colorContext;
    }

    private static int[] fill(int value) {
        int[] values = new int[NUMBER_OF_SPECTRAL_SAMPLES];
        for (int i = 0; i < values.length; i++) {
            values[i] = value;
        }
        return values;
    }

    private static float colorWaveLengthDeltaI() {
        return (float)(COLOR_MAXIMUM_WAVE_LENGTH - COLOR_MINIMUM_WAVE_LENGTH) /
            (float)(NUMBER_OF_SPECTRAL_SAMPLES - 1);
    }

    private static double colorPeakLumensPerWatt() {
        return 683.0 / COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE;
    }

    private static double bBlm(double t) {
        return C2 / 5.0 / t;
    }

    private static double bBsp(double l, double t) {
        return C1 / (l * l * l * l * l * (Math.exp(C2 / (t * l)) - 1.0));
    }

    /**
    Convert a spectrum
    */
    public int setSpectrum(double wlMinimum, double wlMaximum, int ac, String[] av) {
        double scale;
        float[] va = new float[ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 1];
        int i;
        int pos;
        int n;
        int imax;
        int wl;
        double wl0;
        double wlStep;
        double boxPos;
        double boxStep;
        int argumentStartIndex = 0;

        if (av == null || ac < 2 || av.length < ac) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }

        // Check getBoundingBox
        if (wlMaximum <= COLOR_MINIMUM_WAVE_LENGTH || wlMaximum <= wlMinimum || wlMinimum >= COLOR_MAXIMUM_WAVE_LENGTH) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        wlStep = (wlMaximum - wlMinimum) / (ac - 1);
        while (wlMinimum < COLOR_MINIMUM_WAVE_LENGTH) {
            wlMinimum += wlStep;
            ac--;
            argumentStartIndex++;
        }
        while (wlMaximum > COLOR_MAXIMUM_WAVE_LENGTH) {
            wlMaximum -= wlStep;
            ac--;
        }
        imax = ac; // Box filter if necessary
        boxPos = 0;
        boxStep = 1;
        if (wlStep < (double)colorWaveLengthDeltaI()) {
            imax = (int)Math.round((wlMaximum - wlMinimum) / colorWaveLengthDeltaI() + (1 - Numeric.EPSILON));
            boxPos = (wlMinimum - COLOR_MINIMUM_WAVE_LENGTH) / colorWaveLengthDeltaI();
            boxStep = wlStep / colorWaveLengthDeltaI();
            wlStep = colorWaveLengthDeltaI();
        }
        scale = 0.0; // Get values and maximum
        pos = 0;
        for (i = 0; i < imax; i++) {
            va[i] = 0.0f;
            n = 0;
            while (boxPos < i + 0.5 && pos < ac) {
                String value = av[argumentStartIndex + pos];
                if (!TokenValidationContext.isFloat(value)) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                va[i] += Float.parseFloat(value);
                pos++;
                n++;
                boxPos += boxStep;
            }
            if (n > 1) {
                va[i] /= (float)n;
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
        spectralStraightSum = 0; // Convert to our spacing
        wl0 = wlMinimum;
        pos = 0;
        for (i = 0, wl = COLOR_MINIMUM_WAVE_LENGTH; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++, wl += (int)colorWaveLengthDeltaI()) {
            if (wl < wlMinimum || wl > wlMaximum) {
                straightSamples[i] = 0;
            }
            else {
                while (wl0 + wlStep < wl + Numeric.EPSILON) {
                    wl0 += wlStep;
                    pos++;
                }
                if (wl + Numeric.EPSILON >= wl0 && wl - Numeric.EPSILON <= wl0) {
                    straightSamples[i] = (short)Math.round(scale * va[pos] + 0.5);
                }
                else {
                    // Interpolate if necessary
                    int pos1 = Math.min(pos + 1, va.length - 1);
                    straightSamples[i] = (short)Math.round(0.5 + scale / wlStep *
                        (va[pos] * (wl0 + wlStep - wl) +
                            va[pos1] * (wl - wl0)));
                }
                spectralStraightSum += straightSamples[i];
            }
        }
        flags = (short)(COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG);
        clock++;
        return ParseErrorContext.MGF_OK;
    }

    /**
    Set black body spectrum
    */
    public int setBlackBodyTemperature(double tk) {
        double sf;
        double wl;

        if (tk < 1000) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        wl = bBlm(tk);
        // Scale factor based on peak
        if (wl < COLOR_MINIMUM_WAVE_LENGTH * 1e-9) {
            wl = COLOR_MINIMUM_WAVE_LENGTH * 1e-9;
        }
        else if (wl > COLOR_MAXIMUM_WAVE_LENGTH * 1e-9) {
            wl = COLOR_MAXIMUM_WAVE_LENGTH * 1e-9;
        }
        sf = ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / bBsp(wl, tk);
        spectralStraightSum = 0;
        for (int i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
            wl = (COLOR_MINIMUM_WAVE_LENGTH + (float)i * colorWaveLengthDeltaI()) * 1e-9;
            spectralStraightSum += straightSamples[i] = (short)Math.round(sf * bBsp(wl, tk) + 0.5);
        }
        flags = (short)(COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG);
        clock++;
        return ParseErrorContext.MGF_OK;
    }

    /**
    Convert color representations
    */
    public void fixColorRepresentation(int fl) {
        double x;
        double y;
        double z;
        int i;

        fl &= ~flags; // Ignore what's done
        if (fl == 0) {
            // Everything's done!
            return;
        }
        if ((flags & (COLOR_XY_IS_SET_FLAG | COLOR_SPECTRUM_IS_SET_FLAG)) == 0) {
            // Nothing set!
            copy(ColorContext.DEFAULT_COLOR_CONTEXT);
        }
        if ((fl & COLOR_XY_IS_SET_FLAG) != 0) {
            // spec -> cxy *
            x = 0.0;
            y = 0.0;
            z = 0.0;
            for (i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                x += cie_xf.straightSamples[i] * straightSamples[i];
                y += cie_yf.straightSamples[i] * straightSamples[i];
                z += cie_zf.straightSamples[i] * straightSamples[i];
            }
            x /= (double)cie_xf.spectralStraightSum;
            y /= (double)cie_yf.spectralStraightSum;
            z /= (double)cie_zf.spectralStraightSum;
            z += x + y;
            cx = (float)(x / z);
            cy = (float)(y / z);
            flags = (short)(flags | COLOR_XY_IS_SET_FLAG);
        }
        else if ((fl & COLOR_SPECTRUM_IS_SET_FLAG) != 0) {
            // cxy -> spec
            x = cx;
            y = cy;
            z = 1.0 - x - y;
            spectralStraightSum = 0;
            for (i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                straightSamples[i] = (short)Math.round(x * cie_xp.straightSamples[i] + y * cie_yp.straightSamples[i]
                    + z * cie_zp.straightSamples[i] + 0.5);
                if (straightSamples[i] < 0) {
                    // Out of gamut!
                    straightSamples[i] = 0;
                }
                else {
                    spectralStraightSum += straightSamples[i];
                }
            }
            flags = (short)(flags | COLOR_SPECTRUM_IS_SET_FLAG);
        }
        if ((fl & COLOR_EFFICACY_FLAG) != 0) {
            // Compute efficacy
            if ((flags & COLOR_SPECTRUM_IS_SET_FLAG) != 0) {
                // From spectrum
                y = 0.0;
                for (i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                    y += cie_yf.straightSamples[i] * straightSamples[i];
                }
                eff = (float)(colorPeakLumensPerWatt() * y / (double)spectralStraightSum);
            }
            else {
                // flags & C_CS_XY from (x,y)
                eff = (float)(cx * cie_xf.eff + cy * cie_yf.eff +
                    (1.0 - cx - cy) * cie_zf.eff);
            }
            flags = (short)(flags | COLOR_EFFICACY_FLAG);
        }
    }

    /**
    Mix two colors according to weights given
    */
    public void mixColors(
        double w1,
        ColorContext c1,
        double w2,
        ColorContext c2) {
        double scale;
        float[] cMix = new float[ColorContext.NUMBER_OF_SPECTRAL_SAMPLES];

        if (((c1.flags | c2.flags) & COLOR_DEFINED_WITH_SPECTRUM_FLAG) != 0) {
            int i;
            // Spectral mixing
            c1.fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
            c2.fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
            w1 /= c1.eff * (float)c1.spectralStraightSum;
            w2 /= c2.eff * (float)c2.spectralStraightSum;
            scale = 0.0;
            for (i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                cMix[i] = (float)(w1 * c1.straightSamples[i] + w2 * c2.straightSamples[i]);
                if (cMix[i] > scale) {
                    scale = cMix[i];
                }
            }
            scale = ColorContext.COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
            spectralStraightSum = 0;
            for (i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                spectralStraightSum += straightSamples[i] = (short)Math.round(scale * cMix[i] + 0.5);
            }
            flags = (short)(COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG);
        }
        else {
            // CIE xy mixing
            c1.fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
            c2.fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
            scale = w1 / c1.cy + w2 / c2.cy;
            if (scale == 0.0) {
                return;
            }
            scale = 1.0 / scale;
            cx = (float)((c1.cx * w1 / c1.cy + c2.cx * w2 / c2.cy) * scale);
            cy = (float)((w1 + w2) * scale);
            flags = (short)(COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG);
        }
    }
}
