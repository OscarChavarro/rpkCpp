package vsdk.toolkit.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.environment.material.PhongMaterial;

/**
RenderPark's view of the scattering of a material (its split BSDF: the
reflection, the transmission and the texture): the evaluation, sampling and
density functions the raytracers call, with RenderPark's argument types.
Every function converts its arguments (Vector3D to Vector3Df, the shading
context to a SurfaceShadingPoint, the media to their VITRAL materials) and
answers through the VITRAL PhongMaterial; the scattering model itself lives
in VITRAL (plan step 2b). The material and the media are the ones of the
RpkMaterials that own these views, so a view identifies its material.

Read-only after construction; does not own the PhongMaterial.
*/
public class PhongBidirectionalScatteringDistributionFunction {
    private final PhongMaterial material;

    private static PhongMaterial mediumOf(PhongBidirectionalScatteringDistributionFunction bsdf) {
        return bsdf != null ? bsdf.getMaterial() : null;
    }

    public PhongBidirectionalScatteringDistributionFunction(PhongMaterial inMaterial) {
        material = inMaterial;
    }

    public PhongMaterial getMaterial() {
        return material;
    }

    /**
    No material defines its own shading frame: callers build a default one
    */
    public static boolean bsdfShadingFrame(ShadingContext context, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    /**
    Scattered power (diffuse/glossy/specular reflectance and/or transmittance) according to flags
    */
    public ColorRgb splitBsdfScatteredPower(ShadingContext context, int flags) {
        final Vector3Df textureCoordinate = MaterialBoundary.toVitral(context.getTexCoord());
        return material.scatteredPower(
            flags, context.hasFlag(ShadingContextFlag.SHCTX_TEXTURE_COORDINATE) ? textureCoordinate : null);
    }

    public boolean splitBsdfIsTextured() {
        return material.isTextured();
    }

    public void indexOfRefraction(RefractionIndex index) {
        index.set(material.getRefractionIndex(), material.getRefractionIndexImaginary());
    }

    public Vector3D sample(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction)
    {
        return MaterialBoundary.fromVitral(material.scatteringSample(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf), MaterialBoundary.toVitral(in),
            doRussianRoulette != 0, flags, x1, x2, probabilityDensityFunction));
    }

    public ColorRgb evaluate(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags)
    {
        return material.scatteringEvaluate(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            MaterialBoundary.toVitral(in), MaterialBoundary.toVitral(out), flags);
    }

    public void evaluateProbabilityDensityFunction(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR)
    {
        material.scatteringDensity(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            MaterialBoundary.toVitral(in), MaterialBoundary.toVitral(out), flags,
            probabilityDensityFunction, probabilityDensityFunctionRR);
    }

    public ColorRgb bsdfEvalComponents(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        ColorRgb[] colArray)
    {
        return material.scatteringEvaluateComponents(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            MaterialBoundary.toVitral(in), MaterialBoundary.toVitral(out), flags, colArray);
    }
}
