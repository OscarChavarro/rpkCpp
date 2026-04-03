/**
4D vector sampling
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Faure;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Halton;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter31;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.ScrambledHalton;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Sobol;

public class Sample4d {
    private static Sampler4DSequence seq = Sampler4DSequence.RANDOM;

    private static final String RANDOM_NAME = "drand48";
    private static final String HALTON_NAME = "Halton";
    private static final String SCRAMBLED_HALTON_NAME = "ScramHalton";
    private static final String SOBOL_NAME = "sobol";
    private static final String ORIGINAL_FAURE_NAME = "faure";
    private static final String GENERALIZED_FAURE_NAME = "GFaure";
    private static final String NIEDERREITER_NAME = "Nied";
    private static final String UNKNOWN_NAME = "Unknown";

    private static String sequenceName(Sampler4DSequence sequence) {
        switch ( sequence ) {
            case RANDOM:
                return RANDOM_NAME;
            case HALTON:
                return HALTON_NAME;
            case SCRAMBLED_HALTON:
                return SCRAMBLED_HALTON_NAME;
            case SOBOL:
                return SOBOL_NAME;
            case ORIGINAL_FAURE:
                return ORIGINAL_FAURE_NAME;
            case GENERALIZED_FAURE:
                return GENERALIZED_FAURE_NAME;
            case NIEDERREITER:
                return NIEDERREITER_NAME;
            default:
                return UNKNOWN_NAME;
        }
    }

    /**
Also initialises the sequence
*/
    public static void
    setSequence4D(Sampler4DSequence sequence) {
        seq = sequence;
        switch ( seq ) {
            case SOBOL:
                Sobol.initSobol(4);
                break;
            case ORIGINAL_FAURE:
                Faure.initOriginalFaureSequence(4);
                break;
            case GENERALIZED_FAURE:
                Faure.initGeneralizedFaureSequence(4);
                break;
            default:
                break;
        }
    }

    /**
Returns 4D sample with given index from current sequence. When the
current sequence is 'random', the index is not used
*/
    public static double[]
    sample4D(long seed) {
        double[] xi = new double[4];
        long[] zeta;
        double[] xx;

        switch ( seq ) {
            case RANDOM:
                xi[0] = Math.random();
                xi[1] = Math.random();
                xi[2] = Math.random();
                xi[3] = Math.random();
                break;
            case HALTON:
                xi[0] = Halton.Halton2((int)seed);
                xi[1] = Halton.Halton3((int)seed);
                xi[2] = Halton.Halton5((int)seed);
                xi[3] = Halton.Halton7((int)seed);
                break;
            case SCRAMBLED_HALTON:
                xx = ScrambledHalton.scrambledHalton(seed, 4);
                xi[0] = xx[0];
                xi[1] = xx[1];
                xi[2] = xx[2];
                xi[3] = xx[3];
                break;
            case SOBOL:
                xx = Sobol.sobol((int)seed);
                xi[0] = xx[0];
                xi[1] = xx[1];
                xi[2] = xx[2];
                xi[3] = xx[3];
                break;
            case ORIGINAL_FAURE:
            case GENERALIZED_FAURE:
                xx = Faure.faure((int)seed);
                xi[0] = xx[0];
                xi[1] = xx[1];
                xi[2] = xx[2];
                xi[3] = xx[3];
                break;
            case NIEDERREITER:
                zeta = Niederreiter31.niederreiter31(seed);
                xi[0] = zeta[0] * Niederreiter31.RECIP;
                xi[1] = zeta[1] * Niederreiter31.RECIP;
                xi[2] = zeta[2] * Niederreiter31.RECIP;
                xi[3] = zeta[3] * Niederreiter31.RECIP;
                break;
            default:
                Error.fatal(-1, "Sample4d::sample4D", "QMC Sequence %s not yet implemented", sequenceName(seq));
        }

        return xi;
    }

    /**
The following routines are safe with Sample4D(), which calls only
31-bit sequences (including 31-bit Niederreiter sequence). If
you are looking for such a routine to use directly in conjunction
with the routine Niederreiter::Nied() or Niederreiter::NextNiedInRange(), you should use
the Niederreiter::foldSample() routine in Niederreiter.h instead.
Niederreiter::Nied() and Niederreiter::NextNiedInRange() are 63-bit unless compiled without
'unsigned long long' support
*/
    public static void
    foldSampleU(long[] xi1, long[] xi2) {
        Niederreiter31.foldSample31(xi1, xi2);
    }

    public static void
    foldSampleF(double[] xi1, double[] xi2) {
        long zeta1 = (long)(xi1[0] * Niederreiter31.RECIP1);
        long zeta2 = (long)(xi2[0] * Niederreiter31.RECIP1);
        Sample4d.foldSampleU(new long[] {zeta1}, new long[] {zeta2});
        xi1[0] = zeta1 * Niederreiter31.RECIP;
        xi2[0] = zeta2 * Niederreiter31.RECIP;
    }
}
