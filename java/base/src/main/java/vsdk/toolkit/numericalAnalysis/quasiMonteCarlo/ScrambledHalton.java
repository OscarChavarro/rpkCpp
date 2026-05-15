package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;
/**
Scrambled halton quasiMonteCarlo sample sequence
*/


/**
Scrambled Halton quasiMonteCarlo sequence
*/
public class ScrambledHalton {
    private static final int MAX_DIM = 10;
    private static final int[] prime = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    private static final double[] sample = new double[MAX_DIM];

    /**
    Scrambled halton quasiMonteCarlo sample sequence
    */
    public static double[] scrambledHalton(long nextN, int dim) {
        for (int i = 0; i < dim; i++) {
            int b = prime[i]; // Each component uses different base
            double bi = 1.0 / b;
            double fi = 0;
            double bp = 1;
            int m = 0;

            for (long j = nextN; j > 0; j /= b) {
                bp = bp * bi;
                int a = (int)(j % b); // Variable "a" is m-th digit from b-ary representation of nextN
                a = (a + m) % b; // Permutation of variable "a", Warnock's method
                fi = fi + a * bp;
                m += 1;
            }
            sample[i] = fi;
        }
        return sample;
    }
}
