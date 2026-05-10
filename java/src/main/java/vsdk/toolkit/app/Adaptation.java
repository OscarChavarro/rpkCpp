package vsdk.toolkit.app;

import java.util.ArrayList;
import java.util.Arrays;
import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMapAdaptationMethod;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Estimate static adaptation luminance in the current scene
*/
public final class Adaptation {
    private interface PatchRadianceEstimateFn {
        ColorRgb apply(Patch patch);
    }

    private static int numEntries = 0;
    private static double logAreaLum = 0.0;
    private static LuminanceArea[] lumArea = null;
    private static int lumAreaIndex = 0;
    private static float lumMin = Float.MAX_VALUE; // Note Numeric::HUGE_FLOAT_VALUE; will cause an issue here
    private static float lumMax = 0.0f;
    private static PatchRadianceEstimateFn patchRadianceEstimate = null;

    private Adaptation() {
    }

    /**
A-priori estimate of a patch's radiance
*/
    private static ColorRgb initRadianceEstimate(Patch patch) {
        int allXxdfComponents = XxdfComponentFlag.DIFFUSE_COMPONENT
            | XxdfComponentFlag.GLOSSY_COMPONENT
            | XxdfComponentFlag.SPECULAR_COMPONENT;
        int allBsdfComponents = BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT;

        ColorRgb E = PatchVisitor.averageEmittance(patch, allXxdfComponents);
        ColorRgb R = PatchVisitor.averageNormalAlbedo(patch, allBsdfComponents);
        ColorRgb radiance = new ColorRgb();

        radiance.scalarProduct(R, Statistics.instance().radiance.estimatedAverageRadiance);
        radiance.addScaled(radiance, (1.0f / (float)Math.PI), E);
        return radiance;
    }

    private static float patchBrightnessEstimate(Patch patch) {
        ColorRgb radiance = patchRadianceEstimate.apply(patch);
        float brightness = Cie.spectrumLuminance(radiance.r, radiance.g, radiance.b);
        if ( brightness < Numeric.EPSILON_FLOAT ) {
            brightness = Numeric.EPSILON_FLOAT;
        }
        return brightness;
    }

    private static void patchComputeLogAreaLum(Patch patch) {
        float brightness = Adaptation.patchBrightnessEstimate(patch);
        // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
        logAreaLum += patch.area * Math.log(brightness);
    }

    private static void patchFillLumArea(Patch patch) {
        float brightness = Adaptation.patchBrightnessEstimate(patch);

        LuminanceArea entry = lumArea[lumAreaIndex];
        entry.luminance = brightness;
        entry.area = patch.area;

        lumMin = Math.min(lumMin, entry.luminance);
        lumMax = Math.max(lumMax, entry.luminance);

        lumAreaIndex++;
        numEntries++;
    }

    /**
Computes the static adaptation luminance value choosing the median value
of area-weighted luminance values. Needs a correct
Statistics::instance().radiance.totalArea.
*/
    private static float meanAreaWeightedLuminance(LuminanceArea[] pairs, int numPairs) {
        if ( numPairs <= 0 ) {
            return 0.0f;
        }

        float areaMax = Statistics.instance().radiance.totalArea / 2.0f;
        float areaCnt = 0.0f;
        int pairIndex = 0;

        Arrays.sort(pairs, 0, numPairs, (la1, la2) -> Float.compare(la1.luminance, la2.luminance));

        while ( pairIndex < numPairs && areaCnt < areaMax ) {
            areaCnt += pairs[pairIndex].area;
            pairIndex++;
        }

        if ( pairIndex == 0 ) {
            return pairs[0].luminance;
        }
        return pairs[pairIndex - 1].luminance;
    }

    /**
Estimates adaptation luminance in the current scene using the current
adaption estimation method in toneMapOptions.staticAdaptationMethod
'patch_radiance' is a pointer to a routine that computes the radiance
emitted by a patch. The result is filled in toneMapOptions.realWorldAdaptionLuminance
*/
    private static void estimateSceneAdaptation(
        PatchRadianceEstimateFn patchRadiance,
        ArrayList<Patch> scenePatches,
        ToneMappingContext toneMapOptions)
    {
        patchRadianceEstimate = patchRadiance;

        switch ( toneMapOptions.staticAdaptationMethod ) {
            case TMA_NONE:
                break;
            case TMA_AVERAGE: {
                // Gibson's static adaptation after [TUMB1999b]
                logAreaLum = 0.0;
                for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
                    Adaptation.patchComputeLogAreaLum(scenePatches.get(i));
                }
                // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
                toneMapOptions.realWorldAdaptionLuminance =
                    (float)Math.exp((float)logAreaLum / Statistics.instance().radiance.totalArea + 0.84f);
                break;
            }
            case TMA_MEDIAN: {
                // Static adaptation inspired by [TUMB1999b]
                int pairCount = Statistics.instance().reader.numberOfPatches;
                if ( pairCount <= 0 ) {
                    pairCount = scenePatches == null ? 0 : scenePatches.size();
                }
                LuminanceArea[] la = new LuminanceArea[pairCount];
                for ( int i = 0; i < pairCount; i++ ) {
                    la[i] = new LuminanceArea();
                }

                lumArea = la;
                lumAreaIndex = 0;
                numEntries = 0;
                for ( int i = 0; scenePatches != null && i < scenePatches.size() && lumAreaIndex < pairCount; i++ ) {
                    Adaptation.patchFillLumArea(scenePatches.get(i));
                }
                toneMapOptions.realWorldAdaptionLuminance = Adaptation.meanAreaWeightedLuminance(la, numEntries);
                break;
            }
            default:
                Error.error("sceneBuilderComputeStats", "unknown static adaptation method %s", toneMapOptions.staticAdaptationMethod);
        }
    }

    /**
Same as Adaptation::estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
Used when loading a new scene
*/
    public static void initSceneAdaptation(
        ArrayList<Patch> scenePatches,
        ToneMappingContext toneMapOptions)
    {
        Adaptation.estimateSceneAdaptation(Adaptation::initRadianceEstimate, scenePatches, toneMapOptions);
    }
}
