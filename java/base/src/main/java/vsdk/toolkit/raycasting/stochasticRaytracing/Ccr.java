/**
Constant Control Radiosity
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class Ccr {
    private Ccr() {
    }

    @FunctionalInterface
    public interface GetRadianceCallback {
        ColorRgb[] apply(StochasticRadiosityElement element);
    }

    @FunctionalInterface
    public interface GetScalingCallback {
        ColorRgb apply(StochasticRadiosityElement element);
    }

    private static final int NUMBER_OF_INTERVALS = 10;
    private static GetRadianceCallback getRadianceCallback = null;
    private static GetScalingCallback getScalingCallback = null;

    static void initialControlRadiosityRecursive(
        StochasticRadiosityElement element,
        ColorRgb minRad,
        ColorRgb maxRad,
        ColorRgb fMin,
        ColorRgb fMax,
        ColorRgb totalFluxColor,
        ColorRgb maxRadColor,
        double[] area)
    {
        if ( element.regularSubElements == null ) {
            // Trivial case
            ColorRgb rad = getRadianceCallback.apply(element)[0];
            float weightedArea = element.area;
            if ( StochasticRelaxation.activeState().importanceDriven != 0 &&
                 StochasticRelaxation.activeState().method != StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD ) {
                weightedArea *= (element.importance - element.sourceImportance); // Multiply with received importance
            }
            // factor M_PI is omitted everywhere
            totalFluxColor.addScaled(totalFluxColor, weightedArea, rad);
            area[0] += weightedArea;
            maxRadColor.maximum(maxRadColor, rad);
        } else {
            // Recursive case
            for ( int i = 0; i < 4; i++ ) {
                initialControlRadiosityRecursive(
                    (StochasticRadiosityElement)element.regularSubElements[i],
                    minRad,
                    maxRad,
                    fMin,
                    fMax,
                    totalFluxColor,
                    maxRadColor,
                    area);
            }
        }
    }

    /**
Initial guess for constant control radiance value
*/
    static void initialControlRadiosity(
        ColorRgb minRad,
        ColorRgb maxRad,
        ColorRgb fMin,
        ColorRgb fMax,
        ArrayList<Patch> scenePatches)
    {
        ColorRgb totalFluxColor = new ColorRgb();
        ColorRgb maxRadColor = new ColorRgb();
        double[] area = new double[] {0.0};
        totalFluxColor.clear();
        maxRadColor.clear();

        // Initial interval: 0 ... maxRadColor
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            initialControlRadiosityRecursive(
                McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)),
                minRad,
                maxRad,
                fMin,
                fMax,
                totalFluxColor,
                maxRadColor,
                area);
        }

        minRad.clear();
        fMin.set(totalFluxColor.getR(), totalFluxColor.getG(), totalFluxColor.getB());

        maxRad.set(maxRadColor.getR(), maxRadColor.getG(), maxRadColor.getB());
        fMax.scaledCopy((float)area[0], maxRadColor);
        fMax.subtract(fMax, totalFluxColor);
    }

    static void refineComponent(
        float[] minRad,
        float[] maxRad,
        float[] fMin,
        float[] fMax,
        float[] f,
        float[] rad)
    {
        int iMin;

        // Find sub-interval containing the minimum
        fMax[0] = f[0];
        fMin[0] = f[0];
        iMin = 0;
        for ( int i = 1; i <= NUMBER_OF_INTERVALS; i++ ) {
            if ( f[i] < fMin[0] ) {
                fMin[0] = f[i];
                iMin = i;
            }
            if ( f[i] > fMax[0] ) {
                fMax[0] = f[i];
            }
        }

        if ( iMin == 0 ) {
            // First sub-interval contains minimum
            minRad[0] = rad[0];
            maxRad[0] = rad[1];
        } else if ( iMin == NUMBER_OF_INTERVALS ) {
            // Last sub-interval contains minimum
            minRad[0] = rad[NUMBER_OF_INTERVALS - 1];
            maxRad[0] = rad[NUMBER_OF_INTERVALS];
        } else {
            if ( f[iMin - 1] < f[iMin + 1] ) {
                // Sub-interval left of iMin contains minimum
                minRad[0] = rad[iMin - 1];
                maxRad[0] = rad[iMin];
            } else {
                // Sub-interval right of iMin
                minRad[0] = rad[iMin];
                maxRad[0] = rad[iMin + 1];
            }
        }
    }

    static void refineControlRadiosityRecursive(
        StochasticRadiosityElement element,
        ColorRgb colorOne,
        ColorRgb[] rad,
        ColorRgb[] f)
    {
        if ( element.regularSubElements == null ) {
            // Trivial case
            ColorRgb B = getRadianceCallback.apply(element)[0];
            ColorRgb s = getScalingCallback != null ? getScalingCallback.apply(element) : colorOne;
            float weightedArea = element.area;
            if ( StochasticRelaxation.activeState().importanceDriven != 0 &&
                 StochasticRelaxation.activeState().method !=
                 StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD ) {
                weightedArea *= (element.importance - element.sourceImportance); /* multiply with received importance */
            }
            for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
                ColorRgb t = new ColorRgb();
                t.scalarProduct(s, rad[i]);
                t.subtract(B, t);
                t.abs();
                f[i].addScaled(f[i], weightedArea, t);
            }
        } else {
            // Recursive case
            for ( int i = 0; i < 4; i++ ) {
                refineControlRadiosityRecursive((StochasticRadiosityElement)element.regularSubElements[i], colorOne, rad, f);
            }
        }
    }

    /**
Finds sub-interval containing optimal constant control radiosity value
Uses regular interval subdivision (generalisation of the bisection
method). Does so component wise
*/
    static void refineControlRadiosity(
        ColorRgb minRad,
        ColorRgb maxRad,
        ColorRgb fMin,
        ColorRgb fMax,
        ArrayList<Patch> scenePatches)
    {
        ColorRgb colorOne = new ColorRgb();
        ColorRgb[] f = new ColorRgb[NUMBER_OF_INTERVALS + 1];
        ColorRgb[] rad = new ColorRgb[NUMBER_OF_INTERVALS + 1];
        ColorRgb d = new ColorRgb();

        colorOne.setMonochrome(1.0f);

        // Initialisations. rad[i] = radiosity at boundary i
        d.subtract(maxRad, minRad);
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            f[i] = new ColorRgb();
            f[i].clear();
            rad[i] = new ColorRgb();
            rad[i].addScaled(minRad, (float)i / (float)NUMBER_OF_INTERVALS, d);
        }

        // Determine value of F(beta) = sum_i (area_i * java::Math::abs(B_i - beta)) on
        // a regular subdivision of the interval
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            refineControlRadiosityRecursive(
                McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)),
                colorOne,
                rad,
                f);
        }

        // Find sub-interval containing optimal control radiosity (component-wise)
        for ( int s = 0; s < 3; s++ ) {
            float[] fc = new float[NUMBER_OF_INTERVALS + 1];
            float[] radC = new float[NUMBER_OF_INTERVALS + 1];
            for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
                // Copy components
                switch ( s ) {
                    case 0:
                        fc[i] = (float)f[i].getR();
                        radC[i] = (float)rad[i].getR();
                        break;
                    case 1:
                        fc[i] = (float)f[i].getG();
                        radC[i] = (float)rad[i].getG();
                        break;
                    case 2:
                        fc[i] = (float)f[i].getB();
                        radC[i] = (float)rad[i].getB();
                        break;
                    default:
                        break;
                }
            }
            switch ( s ) {
                case 0: {
                    float[] min = new float[] {(float)minRad.getR()};
                    float[] max = new float[] {(float)maxRad.getR()};
                    float[] fMinC = new float[] {(float)fMin.getR()};
                    float[] fMaxC = new float[] {(float)fMax.getR()};
                    refineComponent(min, max, fMinC, fMaxC, fc, radC);
                    minRad.getR() = min[0];
                    maxRad.getR() = max[0];
                    fMin.getR() = fMinC[0];
                    fMax.getR() = fMaxC[0];
                    break;
                }
                case 1: {
                    float[] min = new float[] {(float)minRad.getG()};
                    float[] max = new float[] {(float)maxRad.getG()};
                    float[] fMinC = new float[] {(float)fMin.getG()};
                    float[] fMaxC = new float[] {(float)fMax.getG()};
                    refineComponent(min, max, fMinC, fMaxC, fc, radC);
                    minRad.getG() = min[0];
                    maxRad.getG() = max[0];
                    fMin.getG() = fMinC[0];
                    fMax.getG() = fMaxC[0];
                    break;
                }
                case 2: {
                    float[] min = new float[] {(float)minRad.getB()};
                    float[] max = new float[] {(float)maxRad.getB()};
                    float[] fMinC = new float[] {(float)fMin.getB()};
                    float[] fMaxC = new float[] {(float)fMax.getB()};
                    refineComponent(min, max, fMinC, fMaxC, fc, radC);
                    minRad.getB() = min[0];
                    maxRad.getB() = max[0];
                    fMin.getB() = fMinC[0];
                    fMax.getB() = fMaxC[0];
                    break;
                }
                default:
                    break;
            }
        }
    }

    /**
Determines and returns optimal constant control radiosity value for
the given radiance distribution: this is, the value of beta that
minimises F(beta) = sum over all patches P of P->area times
absolute value of (globalGetRadiance(P) - globalGetScaling(P) * beta).

- getRadiance() returns the radiance to be propagated from a
given ELEMENT.
- getScaling() returns a scale factor (per color component) to be
multiplied with the radiance of the element. If getScaling is a nullptr
pointer, no scaling is applied. Scaling is used in the context of
random walk radiosity
*/
    public static ColorRgb determineControlRadiosity(
        GetRadianceCallback getRadiance,
        GetScalingCallback getScaling,
        ArrayList<Patch> scenePatches)
    {
        ColorRgb minRad = new ColorRgb();
        ColorRgb maxRad = new ColorRgb();
        ColorRgb fMin = new ColorRgb();
        ColorRgb fMax = new ColorRgb();
        ColorRgb beta = new ColorRgb();
        ColorRgb delta = new ColorRgb();
        float eps = 0.001f;
        int sweep = 0;

        getRadianceCallback = getRadiance;
        getScalingCallback = getScaling;
        beta.clear();
        if ( getRadianceCallback == null ) {
            return beta;
        }

        System.err.print("Determining optimal control radiosity value ... ");
        initialControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);

        delta.subtract(fMax, fMin);
        delta.addScaled(delta, (-eps), fMin);
        while ( (delta.maximumComponent() > 0.0f) || sweep < 4 ) {
            sweep++;
            refineControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);
            delta.subtract(fMax, fMin);
            delta.addScaled(delta, (-eps), fMin);
        }

        beta.add(minRad, maxRad);
        beta.scale(0.5f);
        beta.print(System.err);
        System.err.printf(" (%g lux)", Math.PI * Cie.spectrumLuminance(beta.getR(), beta.getG(), beta.getB()));
        System.err.printf("\n");
        return beta;
    }
}
