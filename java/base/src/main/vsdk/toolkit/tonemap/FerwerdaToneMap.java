package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgbMutable;

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/
public final class FerwerdaToneMap extends ToneMap {
    private final ColorRgbMutable sf;
    private float msf;
    private float pmComp;
    private float pmDisplay;
    private float smComp;
    private float smDisplay;
    private float lda;

    public FerwerdaToneMap() {
        sf = new ColorRgbMutable(0.062F, 0.608F, 0.330F);
        msf = 0.0F;
        pmComp = 0.0F;
        pmDisplay = 0.0F;
        smComp = 0.0F;
        smDisplay = 0.0F;
        lda = 0.0F;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        final float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        final float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;

        lda = maximumDisplayLuminance / 2.0F;
        msf = FerwerdaToneMap.mesopicScaleFactor((float) Math.log10(realWorldAdaptionLuminance));
        smComp = FerwerdaToneMap.scotopicOperator((float) Math.log10(lda)) /
                 FerwerdaToneMap.scotopicOperator((float) Math.log10(realWorldAdaptionLuminance));
        pmComp = FerwerdaToneMap.photopicOperator((float) Math.log10(lda)) /
                 FerwerdaToneMap.photopicOperator((float) Math.log10(realWorldAdaptionLuminance));
        smDisplay = smComp / maximumDisplayLuminance;
        pmDisplay = pmComp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        ColorRgbMutable p;
        float sl;
        final float eff = (float) Cie.getLuminousEfficacy();

        radiance.scale(eff);
        p = new ColorRgbMutable(radiance.getR(), radiance.getG(), radiance.getB());
        sl = (float) (smComp * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));
        radiance.scale(pmComp);
        if ( sl > 0.0 ) {
            radiance.addConstant(radiance, sl);
        }
        return radiance;
    }

    @Override
    public ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) {
        final ColorRgbMutable p = new ColorRgbMutable();
        float sl;
        final float eff = (float) Cie.getLuminousEfficacy();

        radiance.scale(eff);
        radiance = new ColorRgbMutable(p.getR(), p.getG(), p.getB());
        sl = (float) (smDisplay * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));
        radiance.scale(pmDisplay);
        if ( sl > 0.0 ) {
            radiance.addConstant(radiance, sl);
        }
        return radiance;
    }

    private static float photopicOperator(float logLa) {
        // Equation [FERW1996](4): piecewise approximation of log(t_p(L_a))
        float r;
        if ( logLa <= -2.6 ) {
            r = -0.72F;
        } else if ( logLa >= 1.9 ) {
            r = logLa - 1.255F;
        } else {
            r = (float) Math.pow(0.249F * logLa + 0.65F, 2.7F) - 0.72F;
        }

        return (float) Math.pow(10.0F, r);
    }

    private static float scotopicOperator(float logLa) {
        // Equation [FERW1996](5): piecewise approximation of log(t_s(L_a))
        float r;
        if ( logLa <= -3.94 ) {
            r = -2.86F;
        } else if ( logLa >= -1.44 ) {
            r = logLa - 0.395F;
        } else {
            r = (float) Math.pow(0.405F * logLa + 1.6F, 2.18F) - 2.86F;
        }

        return (float) Math.pow(10.0F, r);
    }

    private static float mesopicScaleFactor(float logLwa) {
        if ( logLwa < -2.5 ) {
            return 1.0F;
        } else if ( logLwa > 0.8 ) {
            return 0.0F;
        } else {
            return (0.8F - logLwa) / 3.3F;
        }
    }
}
