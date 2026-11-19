package vsdk.toolkit.raycasting.photonMap;

public class DiscreteSampling {
    public static int
    sample(
        double[] probabilities,
        double total,
        double[] x1,
        double[] probabilityDensityFunction)
    {
        int i = 0;
        double sum;
        double left;
        double sample = x1[0] * total;

        sum = probabilities[0];

        while ( sample > sum ) {
            i++;
            sum += probabilities[i];
        }

        // Rescale x_1
        left = sum - probabilities[i];

        x1[0] = ((sample - left) / (sum - left));
        probabilityDensityFunction[0] = probabilities[i] / total;
        return i;
    }
}
