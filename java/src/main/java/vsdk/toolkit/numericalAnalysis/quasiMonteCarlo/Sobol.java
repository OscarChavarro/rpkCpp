package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;

/**
Sobol quasiMonteCarlo sequence
*/
public class Sobol {
    private static final int MAX_DIM = 5;
    private static final int V_MAX = 30;
    private static int dim = 0;
    private static int nextN = 0;
    private static int[] x = new int[MAX_DIM];
    private static int[][] v = new int[MAX_DIM][V_MAX];
    private static int skip = 0;
    private static double recip = 0.0;
    private static final double[] nextSobolSample = new double[MAX_DIM];
    private static final double[] sobolSample = new double[MAX_DIM];

    private static double[] nextSobol() {
        int c = 1;
        int save = nextN;
        while ((save % 2) == 1) {
            c += 1;
            save = save / 2;
        }
        for (int i = 0; i < dim; i++) {
            x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
            nextSobolSample[i] = x[i] * recip;
        }
        nextN += 1;

        return nextSobolSample;
    }

    // Translate n into Gray code
    private static int sobolGray(int n) {
        return n ^ (n >> 1);
    }

    public static double[] sobol(int seed) {
        seed += skip + 1;
        for (int i = 0; i < dim; i++) {
            x[i] = 0;
            int c = 1;
            int gray = Sobol.sobolGray(seed);
            while (gray != 0) {
                if ((gray & 1) != 0) {
                    x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
                }
                c++;
                gray >>= 1;
            }

            sobolSample[i] = x[i] * recip;
        }

        return sobolSample;
    }

    public static void initSobol(int iDim) {
        int[] d = new int[MAX_DIM];
        int[] poly = new int[MAX_DIM];

        nextN = 0;
        dim = iDim;
        recip = 1.0 / Math.pow(2.0, V_MAX);

        // Reading primitive polynomials
        poly[0] = 3;
        d[0] = 1; // x + 1
        poly[1] = 7;
        d[1] = 2; // x^2 + x + 1
        poly[2] = 11;
        d[2] = 3; // x^3 + x + 1
        poly[3] = 19;
        d[3] = 4; // x^4 + x  + 1
        poly[4] = 37;
        d[4] = 5; // x^5 + x^2 + 1

        // Initial values v read in all initial values 1 --> start of sequence worthless!
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < d[i]; j++) {
                v[i][j] = 1;
            }
        }

        // Calculate remainder of v
        for (int i = 0; i < dim; i++) {
            for (int j = d[i]; j < V_MAX; j++) {
                v[i][j] = v[i][j - d[i]];
                int save = poly[i];
                int m = (int)Math.pow(2.0, d[i]);
                for (int k = d[i]; k > 0; k--) {
                    v[i][j] = v[i][j] ^ m * (save % 2) * v[i][j - k];
                    save = save / 2;
                    m = m / 2;
                }
            }
        }

        for (int i = 0; i < dim; i++) {
            x[i] = 0;
        }
        skip = (int)Math.pow(2.0, 6.0); // Not deterministic!
        for (int i = 1; i <= skip; i++) {
            // Discard the beginning of the sequence because the initial values are the same
            Sobol.nextSobol();
        }
    }
}
