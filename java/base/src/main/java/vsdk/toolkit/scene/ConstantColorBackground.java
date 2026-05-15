package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class ConstantColorBackground extends Background {
    private static final float FOUR_PI = 12.56637061435917295385f;
    private static final float INV_FOUR_PI = 0.07957747154594766788f;

    private ColorRgb color;

    public ConstantColorBackground() {
        color = new ColorRgb();
        color.clear();
    }

    public ConstantColorBackground(ColorRgb backgroundColor) {
        color = new ColorRgb(backgroundColor.r, backgroundColor.g, backgroundColor.b);
    }

    @Override
    public ColorRgb radiance(
        Vector3D position,
        Vector3D direction,
        float[] probabilityDensityFunction) {
        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
        }
        return color;
    }

    @Override
    public Vector3D sample(
        Vector3D position,
        float xi1,
        float xi2,
        ColorRgb radianceValue,
        float[] probabilityDensityFunction) {
        final double phi = 2.0 * Math.PI * xi1;
        final float z = 1.0f - 2.0f * xi2;
        final float radialSquared = Math.max(0.0f, 1.0f - z * z);
        final float radius = (float)Math.sqrt(radialSquared);

        if (radianceValue != null) {
            radianceValue.set(color.r, color.g, color.b);
        }
        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
        }

        Vector3D direction = new Vector3D();
        direction.set(
            radius * (float)Math.cos(phi),
            radius * (float)Math.sin(phi),
            z);
        return direction;
    }

    @Override
    public ColorRgb power(Vector3D position) {
        ColorRgb emittedPower = new ColorRgb();
        emittedPower.scaledCopy(ConstantColorBackground.FOUR_PI, color);
        return emittedPower;
    }
}
