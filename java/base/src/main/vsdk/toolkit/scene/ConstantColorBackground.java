package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class ConstantColorBackground extends Background {
    private static final float FOUR_PI = 12.56637061435917295385f;
    private static final float INV_FOUR_PI = 0.07957747154594766788f;

    private ColorRgbMutable color;

    public ConstantColorBackground() {
        color = new ColorRgbMutable(0.0, 0.0, 0.0);
    }

    public ConstantColorBackground(ColorRgbMutable backgroundColor) {
        color = new ColorRgbMutable(backgroundColor.getR(), backgroundColor.getG(), backgroundColor.getB());
    }

    @Override
    public ColorRgbMutable radiance(
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
        ColorRgbMutable radianceValue,
        float[] probabilityDensityFunction) {
        final double phi = 2.0 * Math.PI * xi1;
        final float z = 1.0f - 2.0f * xi2;
        final float radialSquared = Math.max(0.0f, 1.0f - z * z);
        final float radius = (float)Math.sqrt(radialSquared);

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
    public ColorRgbMutable power(Vector3D position) {
        return new ColorRgbMutable(
            ConstantColorBackground.FOUR_PI * color.getR(),
            ConstantColorBackground.FOUR_PI * color.getG(),
            ConstantColorBackground.FOUR_PI * color.getB());
    }
}
