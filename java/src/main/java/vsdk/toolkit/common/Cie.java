package vsdk.toolkit.common;

public final class Cie {
    private static float cieXR = 0.640f;
    private static float cieYR = 0.330f;
    private static float cieXG = 0.290f;
    private static float cieYG = 0.600f;
    private static float cieXB = 0.150f;
    private static float cieYB = 0.060f;
    private static float cieXW = 0.3333333333f;
    private static float cieYW = 0.3333333333f;

    public static final float WHITE_EFFICACY = 183.07f;

    private static float luminousEfficacy = WHITE_EFFICACY;
    private static final float[][] xyz2RgbMat = new float[3][3];
    private static final float[][] rgb2XyzMat = new float[3][3];

    static {
        computeColorConversionTransforms(
            cieXR,
            cieYR,
            cieXG,
            cieYG,
            cieXB,
            cieYB,
            cieXW,
            cieYW);
    }

    private Cie() {
    }

    private static double cieD() {
        return cieXR * (cieYG - cieYB) +
            cieXG * (cieYB - cieYR) +
            cieXB * (cieYR - cieYG);
    }

    private static double cieCrD() {
        return (1.0 / cieYW) *
            (cieXW * (cieYG - cieYB) -
                cieYW * (cieXG - cieXB) +
                cieXG * cieYB - cieXB * cieYG);
    }

    private static double cieCgD() {
        return (1.0 / cieYW) *
            (cieXW * (cieYB - cieYR) -
                cieYW * (cieXB - cieXR) -
                cieXR * cieYB + cieXB * cieYR);
    }

    private static double cieCbD() {
        return (1.0 / cieYW) *
            (cieXW * (cieYR - cieYG) -
                cieYW * (cieXR - cieXG) +
                cieXR * cieYG - cieXG * cieYR);
    }

    private static double cieRf() {
        return cieYR * cieCrD() / cieD();
    }

    private static double cieGf() {
        return cieYG * cieCgD() / cieD();
    }

    private static double cieBf() {
        return cieYB * cieCbD() / cieD();
    }

    private static float gray(float r, float g, float b) {
        return (float)cieRf() * r + (float)cieGf() * g + (float)cieBf() * b;
    }

    private static float luminance(float r, float g, float b) {
        return luminousEfficacy * gray(r, g, b);
    }

    private static void setColorTransform(
        float[][] mat,
        float a,
        float b,
        float c,
        float d,
        float e,
        float f,
        float g,
        float h,
        float i) {
        mat[0][0] = a;
        mat[0][1] = b;
        mat[0][2] = c;
        mat[1][0] = d;
        mat[1][1] = e;
        mat[1][2] = f;
        mat[2][0] = g;
        mat[2][1] = h;
        mat[2][2] = i;
    }

    private static void colorTransform(float[] col, float[][] mat, float[] res) {
        res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
        res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
        res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
    }

    public static void transformColorFromXYZ2RGB(float[] xyz, float[] rgb) {
        colorTransform(xyz, xyz2RgbMat, rgb);
    }

    public static boolean clipGamut(float[] rgb) {
        boolean desaturated = false;
        for (int i = 0; i < 3; i++) {
            if (rgb[i] < 0.0f) {
                rgb[i] = 0.0f;
                desaturated = true;
            }
        }
        return desaturated;
    }

    public static void computeColorConversionTransforms(
        float xr,
        float yr,
        float xg,
        float yg,
        float xb,
        float yb,
        float xw,
        float yw) {
        cieXR = xr;
        cieYR = yr;
        cieXG = xg;
        cieYG = yg;
        cieXB = xb;
        cieYB = yb;
        cieXW = xw;
        cieYW = yw;

        setColorTransform(
            xyz2RgbMat,
            (float)((cieYG - cieYB - cieXB * cieYG + cieYB * cieXG) / cieCrD()),
            (float)((cieXB - cieXG - cieXB * cieYG + cieXG * cieYB) / cieCrD()),
            (float)((cieXG * cieYB - cieXB * cieYG) / cieCrD()),
            (float)((cieYB - cieYR - cieYB * cieXR + cieYR * cieXB) / cieCgD()),
            (float)((cieXR - cieXB - cieXR * cieYB + cieXB * cieYR) / cieCgD()),
            (float)((cieXB * cieYR - cieXR * cieYB) / cieCgD()),
            (float)((cieYR - cieYG - cieYR * cieXG + cieYG * cieXR) / cieCbD()),
            (float)((cieXG - cieXR - cieXG * cieYR + cieXR * cieYG) / cieCbD()),
            (float)((cieXR * cieYG - cieXG * cieYR) / cieCbD()));

        setColorTransform(
            rgb2XyzMat,
            (float)(cieXR * cieCrD() / cieD()),
            (float)(cieXG * cieCgD() / cieD()),
            (float)(cieXB * cieCbD() / cieD()),
            (float)(cieYR * cieCrD() / cieD()),
            (float)(cieYG * cieCgD() / cieD()),
            (float)(cieYB * cieCbD() / cieD()),
            (float)((1.0 - cieXR - cieYR) * cieCrD() / cieD()),
            (float)((1.0 - cieXG - cieYG) * cieCgD() / cieD()),
            (float)((1.0 - cieXB - cieYB) * cieCbD() / cieD()));
    }

    public static float getLuminousEfficacy() {
        return luminousEfficacy;
    }

    public static float spectrumGray(float r, float g, float b) {
        return gray(r, g, b);
    }

    public static float spectrumLuminance(float r, float g, float b) {
        return luminance(r, g, b);
    }
}
