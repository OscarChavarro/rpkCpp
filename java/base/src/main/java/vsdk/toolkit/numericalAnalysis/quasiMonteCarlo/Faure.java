package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;
/**
Faure's quasiMonteCarlo sequences
*/

// PR part representation of x

// diameter[s] is 1e diameter > s

/**
If NO_GRAY is defined, you can't mix NextFaure() and Faure() calls,
but faure() will be faster because it doesn't need to convert to
seed to it's Gray code
*/

/**
Return sample with given index
*/

/**
Initialize for Original Faure sequence
*/

/**
Initialize for generalized Faure sequence
*/


/**
Faure's quasiMonteCarlo sequences + generalized Faure sequences
*/
public class Faure {
    private static final int[] prime = {2, 3, 5, 5, 7, 7, 11, 11, 11, 11};
    private static int[][] ix = new int[FaureSequenceLimits.MAX_DIMENSION][FaureSequenceLimits.MAX_PRIME_DIGITS];
    private static double[] nextFaureSample = new double[FaureSequenceLimits.MAX_DIMENSION];
    private static double[] faureSample = new double[FaureSequenceLimits.MAX_DIMENSION];
    private static int dimension = 0;
    private static int primeBase = 0;
    private static int nextN = 0;
    private static int skip = 0;
    private static int nDigits = 0;
    private static int[][][] generatorMatrix = new int[FaureSequenceLimits.MAX_DIMENSION][FaureSequenceLimits.MAX_PRIME_DIGITS][FaureSequenceLimits.MAX_PRIME_DIGITS];

    private static int setFaureC() {
        // First set up generatorMatrix[0][][] (transposed Pascal matrix)
        for (int j = 0; j < nDigits; j++) {
            for (int k = j; k < nDigits; k++) {
                if (j == 0 || j == k) {
                    generatorMatrix[0][j][k] = 1;
                }
                else {
                    generatorMatrix[0][j][k] = (generatorMatrix[0][j][k - 1] + generatorMatrix[0][j - 1][k - 1]) % primeBase;
                }
            }
        }

        // Use generatorMatrix[0][][] to compose generatorMatrix[i][][]
        // generatorMatrix[0] is overwritten if i=0 -> becomes unit matrix
        for (int i = dimension - 1; i >= 0; i--) {
            for (int j = 0; j < nDigits; j++) {
                for (int k = j; k < nDigits; k++) {
                    generatorMatrix[i][j][k] = (int)((generatorMatrix[0][j][k] * Math.pow(i, k - j)) % primeBase);
                }
            }
        }

        return 0;
    }

    private static int setGFaureC() {
        int[][] p = new int[FaureSequenceLimits.MAX_PRIME_DIGITS][FaureSequenceLimits.MAX_PRIME_DIGITS];

        // Pascal matrix
        for (int j = 0; j < nDigits; j++) {
            p[j][0] = 1;
            p[j][j] = 1;
        }

        for (int j = 1; j < nDigits; j++) {
            for (int k = 1; k < j; k++) {
                p[j][k] = (p[j - 1][k - 1] + p[j - 1][k]) % primeBase;
            }
            for (int k = j + 1; k < nDigits; k++) {
                p[j][k] = 0;
            }
        }

        // [Tezuka95, p179-180]
        for (int i = 0; i < dimension; i++) {
            // Compute generatorMatrix[i]
            for (int m = 0; m < nDigits; m++) {
                for (int n = 0; n < nDigits; n++) {
                    int qMax = m < n ? m : n;
                    generatorMatrix[i][m][n] = 0;
                    for (int q = 0; q <= qMax; q++) {
                        generatorMatrix[i][m][n] =
                            (int)((generatorMatrix[i][m][n] + p[m][q] * p[n][q] * Math.pow(i, m + n - 2 * q)) % primeBase);
                    }
                }
            }
        }

        return 0;
    }

    /**
    If NO_GRAY is defined, you can't mix NextFaure() and Faure() calls,
    but faure() will be faster because it doesn't need to convert to
    seed to it's Gray code
    */
    private static double[] nextFaure() {
        int save;
        double xx;

        save = nextN;
        int k = 1;
        while ((save % primeBase) == (primeBase - 1)) {
            k = k + 1;
            save = save / primeBase;
        }
        for (int i = 0; i < dimension && i < FaureSequenceLimits.MAX_DIMENSION; i++) {
            xx = 0;
            for (int j = nDigits - 1; j >= 0; j--) {
                if (j < FaureSequenceLimits.MAX_PRIME_DIGITS) {
                    ix[i][j] = (ix[i][j] + generatorMatrix[i][j][k - 1]) % primeBase;
                    xx = xx / primeBase + ix[i][j];
                }
            }
            nextFaureSample[i] = xx / primeBase;
        }
        nextN += 1;
        return nextFaureSample;
    }

    /**
    Return sample with given index
    */
    public static double[] faure(int seed) {
        int save;
        double xx;

        nextN = seed + skip + 1;
        for (int i = 0; i < dimension; i++) {
            xx = 0;
            for (int j = nDigits - 1; j >= 0; j--) {
                save = nextN;
                ix[i][j] = 0;
                for (int k = 0; k < nDigits; k++) {
                    ix[i][j] = (ix[i][j] + generatorMatrix[i][j][k] * save) % primeBase;
                    save /= primeBase;
                }
                xx = xx / primeBase + ix[i][j];
            }
            faureSample[i] = xx / primeBase;
        }
        return faureSample;
    }

    /**
    Initialize for Original Faure sequence
    */
    public static void initOriginalFaureSequence(int iDim) {
        dimension = iDim;
        nextN = 0;
        primeBase = prime[dimension - 1];
        nDigits = (int)(Math.log(FaureSequenceLimits.MAX_SEED) / Math.log(primeBase) + 1);
        Faure.setFaureC();
        for (int i = 0; i < dimension; i++) {
            for (int j = 0; j < nDigits; j++) {
                ix[i][j] = 0;
            }
        }

        skip = (int)(Math.pow(primeBase, 4.0) - 1);
        for (int i = 1; i <= skip; i++) {
            // Warm up
            Faure.nextFaure();
        }
    }

    /**
    Initialize for generalized Faure sequence
    */
    public static void initGeneralizedFaureSequence(int iDim) {
        dimension = iDim;
        nextN = 0;
        primeBase = prime[dimension - 1];
        nDigits = (int)(Math.log(FaureSequenceLimits.MAX_SEED) / Math.log(primeBase) + 1);
        Faure.setGFaureC();
        for (int i = 0; i < dimension; i++) {
            for (int j = 0; j < nDigits; j++) {
                ix[i][j] = 0;
            }
        }

        skip = (int)(Math.pow(primeBase, 4.0) - 1);
        for (int i = 1; i <= skip; i++) {
            // Warm up
            Faure.nextFaure();
        }
    }
}
