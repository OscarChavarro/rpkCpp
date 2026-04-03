package vsdk.toolkit.common;

public class StratifiedSampling2D {
    private int xMaxStratum;
    private int yMaxStratum;
    private int xStratum;
    private int yStratum;

    public StratifiedSampling2D(int nrSamples) {
        int[] divs1 = new int[1];
        int[] divs2 = new int[1];
        getNumberOfDivisions(nrSamples, divs1, divs2);
        xMaxStratum = divs1[0];
        yMaxStratum = divs2[0];
        xStratum = 0;
        yStratum = 0;
    }

    public void sample(double[] x1, double[] x2) {
        if (x1 == null || x1.length == 0 || x2 == null || x2.length == 0) {
            throw new IllegalArgumentException("Output arrays must have length >= 1");
        }

        if (yStratum < yMaxStratum && xMaxStratum > 0 && yMaxStratum > 0) {
            x1[0] = (xStratum + Math.random()) / (double)xMaxStratum;
            x2[0] = (yStratum + Math.random()) / (double)yMaxStratum;

            if ((++xStratum) == xMaxStratum) {
                xStratum = 0;
                yStratum++;
            }
        }
        else {
            x1[0] = Math.random();
            x2[0] = Math.random();
        }
    }

    public double[] sample() {
        double[] x1 = new double[1];
        double[] x2 = new double[1];
        sample(x1, x2);
        return new double[] {x1[0], x2[0]};
    }

    private static void getNumberOfDivisions(int samples, int[] divs1, int[] divs2) {
        if (samples <= 0) {
            divs1[0] = 0;
            divs2[0] = 0;
            return;
        }

        divs1[0] = (int)Math.ceil(Math.sqrt(samples));
        divs2[0] = samples / divs1[0];
        while (divs1[0] * divs2[0] != samples && divs1[0] > 1) {
            divs1[0]--;
            divs2[0] = samples / divs1[0];
        }
    }
}
