package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class ConstantColorBackground extends Background {
    private static final double FOUR_PI = 12.56637061435917295385;
    private static final double INV_FOUR_PI = 0.07957747154594766788;

    private ColorRgb color;

    public ConstantColorBackground() {
        color = new ColorRgb(0.0, 0.0, 0.0);
    }

    public ConstantColorBackground(ColorRgb backgroundColor) {
        color = new ColorRgb(backgroundColor.getR(), backgroundColor.getG(), backgroundColor.getB());
    }

    @Override
    public ColorRgb radiance(
        Vector3Dd position,
        Vector3Dd direction,
        double[] probabilityDensityFunction) {
        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
        }
        return color;
    }

    @Override
    public Vector3Dd sample(
        Vector3Dd position,
        double xi1,
        double xi2,
        ColorRgb radianceValue,
        double[] probabilityDensityFunction) {
        final double phi = 2.0 * Math.PI * xi1;
        final double z = 1.0 - 2.0 * xi2;
        final double radialSquared = Math.max(0.0, 1.0 - z * z);
        final double radius = (double)Math.sqrt(radialSquared);

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
        }

        Vector3Dd direction = new Vector3Dd();
        direction = new Vector3Dd(radius * (double)Math.cos(phi), radius * (double)Math.sin(phi), z);
        return direction;
    }

    @Override
    public ColorRgb power(Vector3Dd position) {
        return new ColorRgb(
            ConstantColorBackground.FOUR_PI * color.getR(),
            ConstantColorBackground.FOUR_PI * color.getG(),
            ConstantColorBackground.FOUR_PI * color.getB());
    }
}
