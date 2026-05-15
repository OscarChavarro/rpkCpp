package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/
public final class FerwerdaToneMap extends ToneMap {
    private ColorRgb sf;
    private float msf;
    private float pmComp;
    private float pmDisplay;
    private float smComp;
    private float smDisplay;
    private float lda;

    private static float photopicOperator(float logLa) {
        // Equation [FERW1996](4): piecewise approximation of log(t_p(L_a))
        float r;
        if (logLa <= -2.6) {
            r = -0.72f;
        }
        else if (logLa >= 1.9) {
            r = logLa - 1.255f;
        }
        else {
            r = (float)Math.pow(0.249f * logLa + 0.65f, 2.7) - 0.72f;
        }

        return (float)Math.pow(10.0, r);
    }

    private static float scotopicOperator(float logLa) {
        // Equation [FERW1996](5): piecewise approximation of log(t_s(L_a))
        float r;
        if (logLa <= -3.94) {
            r = -2.86f;
        }
        else if (logLa >= -1.44) {
            r = logLa - 0.395f;
        }
        else {
            r = (float)Math.pow(0.405f * logLa + 1.6f, 2.18) - 2.86f;
        }

        return (float)Math.pow(10.0, r);
    }

    private static float mesopicScaleFactor(float logLwa) {
        if (logLwa < -2.5) {
            return 1.0f;
        }
        else if (logLwa > 0.8) {
            return 0.0f;
        }
        else {
            return (0.8f - logLwa) / 3.3f;
        }
    }

    public FerwerdaToneMap() {
        sf = new ColorRgb(0.062f, 0.608f, 0.330f);
        msf = 0.0f;
        pmComp = 0.0f;
        pmDisplay = 0.0f;
        smComp = 0.0f;
        smDisplay = 0.0f;
        lda = 0.0f;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        lda = maximumDisplayLuminance / 2.0f;

        // Equations [FERW1996](4) and [FERW1996](5): t_p(L_a), t_s(L_a)
        msf = FerwerdaToneMap.mesopicScaleFactor((float)Math.log10(realWorldAdaptionLuminance));
        // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using scotopic t_s from Equation (5)
        smComp = FerwerdaToneMap.scotopicOperator((float)Math.log10(lda)) /
            FerwerdaToneMap.scotopicOperator((float)Math.log10(realWorldAdaptionLuminance));
        // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using photopic t_p from Equation (4)
        pmComp = FerwerdaToneMap.photopicOperator((float)Math.log10(lda)) /
            FerwerdaToneMap.photopicOperator((float)Math.log10(realWorldAdaptionLuminance));
        smDisplay = smComp / maximumDisplayLuminance;
        pmDisplay = pmComp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        ColorRgb p = new ColorRgb();
        float sl;

        // Convert to photometric values
        float eff = Cie.getLuminousEfficacy();
        radiance.scale(eff);

        // Compute the scotopic grayscale shift
        p.set(radiance.r, radiance.g, radiance.b);
        // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
        sl = (float)(smComp * msf * (p.r * sf.r + p.g * sf.g + p.b * sf.b));

        // Scale the photopic luminance
        radiance.scale(pmComp);

        // Eventually, offset by the scotopic luminance
        if (sl > 0.0) {
            radiance.addConstant(radiance, sl);
        }

        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        ColorRgb p = new ColorRgb();
        float sl;

        // Convert to photometric values
        float eff = Cie.getLuminousEfficacy();
        radiance.scale(eff);

        // Compute the scotopic grayscale shift
        radiance.set(p.r, p.g, p.b);
        // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
        sl = (float)(smDisplay * msf * (p.r * sf.r + p.g * sf.g + p.b * sf.b));

        // Scale the photopic luminance
        radiance.scale(pmDisplay);

        // Eventually, offset by the scotopic luminance
        if (sl > 0.0) {
            radiance.addConstant(radiance, sl);
        }

        return radiance;
    }
}
