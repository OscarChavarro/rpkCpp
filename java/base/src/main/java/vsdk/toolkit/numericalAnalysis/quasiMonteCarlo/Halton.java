package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;

/**
Halton quasi Monte Carlo sample generator
*/
public class Halton {
    public static double Halton2(int i) {
        long h;
        long f;

        h = i & 1L;
        f = 2L;
        i >>= 1;
        while (i != 0) {
            h <<= 1;
            h += (i & 1L);
            i >>= 1;
            f <<= 1;
            h <<= 1;
            h += (i & 1L);
            i >>= 1;
            f <<= 1;
            h <<= 1;
            h += (i & 1L);
            i >>= 1;
            f <<= 1;
            h <<= 1;
            h += (i & 1L);
            i >>= 1;
            f <<= 1;
        }

        return (double)h / (double)f;
    }

    public static double Halton3(int i) {
        long h;
        long f;
        long j = i;
        i /= 3;
        h = j - ((i << 1) + i);
        f = 3;
        while (i > 0) {
            long k;
            j = i;
            i /= 3;
            k = h - i;
            h = j + (k << 1) + k;
            f = (f << 1) + f;
        }

        return (double)h / (double)f;
    }

    public static double Halton5(int i) {
        long h;
        long f;
        long j = i;
        i /= 5;
        h = j - ((i << 2) + i);
        f = 5;
        while (i > 0) {
            long k;
            j = i;
            i /= 5;
            k = h - i;
            h = j + (k << 2) + k;
            f = (f << 2) + f;
        }

        return (double)h / (double)f;
    }

    public static double Halton7(int i) {
        long h;
        long f;
        long j = i;
        i /= 7;
        h = j - ((i << 2) + (i << 1) + i);
        f = 7;
        while (i > 0) {
            long k;
            j = i;
            i /= 7;
            k = h - i;
            h = j + (k << 2) + (k << 1) + k;
            f = (f << 2) + (f << 1) + f;
        }

        return (double)h / (double)f;
    }
}
