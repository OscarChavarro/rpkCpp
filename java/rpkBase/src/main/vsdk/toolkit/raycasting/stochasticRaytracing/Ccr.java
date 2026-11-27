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
        ColorRgb[] minRad,
        ColorRgb[] maxRad,
        ColorRgb[] fMin,
        ColorRgb[] fMax,
        ColorRgb[] totalFluxColor,
        ColorRgb[] maxRadColor,
        double[] area)
    {
        if ( element.regularSubElements == null ) {
            // Trivial case
            ColorRgb rad = getRadianceCallback.apply(element)[0];
            double weightedArea = element.area;
            if ( StochasticRelaxation.activeState().importanceDriven != 0 &&
                 StochasticRelaxation.activeState().method != StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD ) {
                weightedArea *= (element.importance - element.sourceImportance); // Multiply with received importance
            }
            // factor M_PI is omitted everywhere
            totalFluxColor[0] = totalFluxColor[0].addScaled(weightedArea, rad);
            area[0] += weightedArea;
            maxRadColor[0] = maxRadColor[0].maximum(rad);
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
        ColorRgb[] minRad,
        ColorRgb[] maxRad,
        ColorRgb[] fMin,
        ColorRgb[] fMax,
        ArrayList<Patch> scenePatches)
    {
        final ColorRgb[] totalFluxColor = new ColorRgb[] {new ColorRgb(0.0, 0.0, 0.0)};
        final ColorRgb[] maxRadColor = new ColorRgb[] {new ColorRgb(0.0, 0.0, 0.0)};
        double[] area = new double[] {0.0};

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

        minRad[0] = new ColorRgb(0.0, 0.0, 0.0);
        fMin[0] = totalFluxColor[0];

        maxRad[0] = maxRadColor[0];
        fMax[0] = maxRadColor[0].multiply(area[0]);
        fMax[0] = fMax[0].subtract(totalFluxColor[0]);
    }

    static void refineComponent(
        double[] minRad,
        double[] maxRad,
        double[] fMin,
        double[] fMax,
        double[] f,
        double[] rad)
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
            double weightedArea = element.area;
            if ( StochasticRelaxation.activeState().importanceDriven != 0 &&
                 StochasticRelaxation.activeState().method !=
                 StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD ) {
                weightedArea *= (element.importance - element.sourceImportance); /* multiply with received importance */
            }
            for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
                ColorRgb t = new ColorRgb();
                t = s.multiply(rad[i]);
                t = B.subtract(t);
                t = t.absolute();
                f[i] = f[i].addScaled(weightedArea, t);
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
        ColorRgb[] minRad,
        ColorRgb[] maxRad,
        ColorRgb[] fMin,
        ColorRgb[] fMax,
        ArrayList<Patch> scenePatches)
    {
        ColorRgb colorOne = new ColorRgb();
        ColorRgb[] f = new ColorRgb[NUMBER_OF_INTERVALS + 1];
        ColorRgb[] rad = new ColorRgb[NUMBER_OF_INTERVALS + 1];
        ColorRgb d = new ColorRgb();

        colorOne = new ColorRgb(1.0, 1.0, 1.0);

        // Initialisations. rad[i] = radiosity at boundary i
        d = maxRad[0].subtract(minRad[0]);
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            f[i] = new ColorRgb();
            f[i] = new ColorRgb(0.0, 0.0, 0.0);
            rad[i] = new ColorRgb();
            rad[i] = minRad[0].addScaled((double)i / (double)NUMBER_OF_INTERVALS, d);
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
        final double[] minValues = new double[] {minRad[0].getR(), minRad[0].getG(), minRad[0].getB()};
        final double[] maxValues = new double[] {maxRad[0].getR(), maxRad[0].getG(), maxRad[0].getB()};
        final double[] fMinValues = new double[] {fMin[0].getR(), fMin[0].getG(), fMin[0].getB()};
        final double[] fMaxValues = new double[] {fMax[0].getR(), fMax[0].getG(), fMax[0].getB()};

        for ( int s = 0; s < 3; s++ ) {
            double[] fc = new double[NUMBER_OF_INTERVALS + 1];
            double[] radC = new double[NUMBER_OF_INTERVALS + 1];
            for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
                // Copy components
                switch ( s ) {
                    case 0:
                        fc[i] = f[i].getR();
                        radC[i] = rad[i].getR();
                        break;
                    case 1:
                        fc[i] = f[i].getG();
                        radC[i] = rad[i].getG();
                        break;
                    case 2:
                        fc[i] = f[i].getB();
                        radC[i] = rad[i].getB();
                        break;
                    default:
                        break;
                }
            }
            final double[] min = new double[] {minValues[s]};
            final double[] max = new double[] {maxValues[s]};
            final double[] fMinC = new double[] {fMinValues[s]};
            final double[] fMaxC = new double[] {fMaxValues[s]};
            refineComponent(min, max, fMinC, fMaxC, fc, radC);
            minValues[s] = min[0];
            maxValues[s] = max[0];
            fMinValues[s] = fMinC[0];
            fMaxValues[s] = fMaxC[0];
        }

        minRad[0] = new ColorRgb(minValues[0], minValues[1], minValues[2]);
        maxRad[0] = new ColorRgb(maxValues[0], maxValues[1], maxValues[2]);
        fMin[0] = new ColorRgb(fMinValues[0], fMinValues[1], fMinValues[2]);
        fMax[0] = new ColorRgb(fMaxValues[0], fMaxValues[1], fMaxValues[2]);
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
        ArrayList<Patch> scenePatches,
        Cie colorSpace)
    {
        final ColorRgb[] minRad = new ColorRgb[] {new ColorRgb()};
        final ColorRgb[] maxRad = new ColorRgb[] {new ColorRgb()};
        final ColorRgb[] fMin = new ColorRgb[] {new ColorRgb()};
        final ColorRgb[] fMax = new ColorRgb[] {new ColorRgb()};
        ColorRgb beta = new ColorRgb();
        ColorRgb delta = new ColorRgb();
        double eps = 0.001;
        int sweep = 0;

        getRadianceCallback = getRadiance;
        getScalingCallback = getScaling;
        beta = new ColorRgb(0.0, 0.0, 0.0);
        if ( getRadianceCallback == null ) {
            return beta;
        }

        System.err.print("Determining optimal control radiosity value ... ");
        initialControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);

        delta = fMax[0].subtract(fMin[0]);
        delta = delta.addScaled((-eps), fMin[0]);
        while ( (delta.maximumComponent() > 0.0) || sweep < 4 ) {
            sweep++;
            refineControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);
            delta = fMax[0].subtract(fMin[0]);
            delta = delta.addScaled((-eps), fMin[0]);
        }

        beta = minRad[0].add(maxRad[0]);
        beta = beta.multiply(0.5);
        System.err.printf("%g %g %g", beta.getR(), beta.getG(), beta.getB());
        System.err.printf(" (%g lux)", Math.PI * colorSpace.spectrumLuminance(beta.getR(), beta.getG(), beta.getB()));
        System.err.printf("\n");
        return beta;
    }
}
