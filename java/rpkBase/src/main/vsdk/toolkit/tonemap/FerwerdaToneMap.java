package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/
public final class FerwerdaToneMap extends ToneMap {
    private final ColorRgb sf;
    private double msf;
    private double pmComp;
    private double pmDisplay;
    private double smComp;
    private double smDisplay;
    private double lda;

    public FerwerdaToneMap() {
        sf = new ColorRgb(0.062, 0.608, 0.330);
        msf = 0.0;
        pmComp = 0.0;
        pmDisplay = 0.0;
        smComp = 0.0;
        smDisplay = 0.0;
        lda = 0.0;
    }

    @Override
    protected void initParameters(ToneMappingContext toneMapOptions) {
        final double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        final double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;

        lda = maximumDisplayLuminance / 2.0;
        msf = FerwerdaToneMap.mesopicScaleFactor((double)Math.log10(realWorldAdaptionLuminance));
        smComp = FerwerdaToneMap.scotopicOperator((double)Math.log10(lda)) /
                 FerwerdaToneMap.scotopicOperator((double)Math.log10(realWorldAdaptionLuminance));
        pmComp = FerwerdaToneMap.photopicOperator((double)Math.log10(lda)) /
                 FerwerdaToneMap.photopicOperator((double)Math.log10(realWorldAdaptionLuminance));
        smDisplay = smComp / maximumDisplayLuminance;
        pmDisplay = pmComp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        // Convert to photometric values and compute the scotopic grayscale shift
        final double eff = (double)getColorSpace().getLuminousEfficacy();
        final ColorRgb p = radiance.multiply(eff);
        double sl;
        // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
        sl = (double)(smComp * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

        // Scale the photopic luminance
        final ColorRgb photopic = p.multiply(pmComp);

        // Eventually, offset by the scotopic luminance
        if ( sl > 0.0 ) {
            return new ColorRgb(photopic.getR() + sl, photopic.getG() + sl, photopic.getB() + sl);
        }

        return photopic;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        final ColorRgb p = new ColorRgb();
        double sl;

        // The legacy display path uses the zero-initialized scotopic color.
        // Keep this behavior while changing the color value type.
        // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
        sl = (double)(smDisplay * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

        // Scale the photopic luminance
        final ColorRgb photopic = p.multiply(pmDisplay);

        // Eventually, offset by the scotopic luminance
        if ( sl > 0.0 ) {
            return new ColorRgb(photopic.getR() + sl, photopic.getG() + sl, photopic.getB() + sl);
        }

        return photopic;
    }

    private static double photopicOperator(double logLa) {
        // Equation [FERW1996](4): piecewise approximation of log(t_p(L_a))
        double r;
        if ( logLa <= -2.6 ) {
            r = -0.72;
        } else if ( logLa >= 1.9 ) {
            r = logLa - 1.255;
        } else {
            r = (double)Math.pow(0.249 * logLa + 0.65, 2.7) - 0.72;
        }

        return (double)Math.pow(10.0, r);
    }

    private static double scotopicOperator(double logLa) {
        // Equation [FERW1996](5): piecewise approximation of log(t_s(L_a))
        double r;
        if ( logLa <= -3.94 ) {
            r = -2.86;
        } else if ( logLa >= -1.44 ) {
            r = logLa - 0.395;
        } else {
            r = (double)Math.pow(0.405 * logLa + 1.6, 2.18) - 2.86;
        }

        return (double)Math.pow(10.0, r);
    }

    private static double mesopicScaleFactor(double logLwa) {
        if ( logLwa < -2.5 ) {
            return 1.0;
        } else if ( logLwa > 0.8 ) {
            return 0.0;
        } else {
            return (0.8 - logLwa) / 3.3;
        }
    }
}
