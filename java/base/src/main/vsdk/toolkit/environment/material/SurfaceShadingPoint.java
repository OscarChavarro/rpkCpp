package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

/**
What the directional functions of PhongMaterial read about the surface point
being shaded: the shading normal and the texture coordinate, each with a flag
telling whether the caller could determine it. The caller finds the point,
its normal and its texture coordinate; the material only reads them.

Immutable value.
*/
public final class SurfaceShadingPoint {
    private final Vector3Df shadingNormal;
    private final Vector3Df textureCoordinate;
    private final boolean shadingNormalKnown;
    private final boolean textureCoordinateKnown;

    public SurfaceShadingPoint(
        Vector3Df inShadingNormal,
        boolean inShadingNormalKnown,
        Vector3Df inTextureCoordinate,
        boolean inTextureCoordinateKnown)
    {
        shadingNormal = inShadingNormal;
        textureCoordinate = inTextureCoordinate;
        shadingNormalKnown = inShadingNormalKnown;
        textureCoordinateKnown = inTextureCoordinateKnown;
    }

    /** Unit normal leaving the surface on the side the scattering is evaluated from */
    public Vector3Df getShadingNormal() {
        return shadingNormal;
    }

    public boolean hasShadingNormal() {
        return shadingNormalKnown;
    }

    /** Texture coordinate (x and y used) */
    public Vector3Df getTextureCoordinate() {
        return textureCoordinate;
    }

    public boolean hasTextureCoordinate() {
        return textureCoordinateKnown;
    }
}
