package vsdk.toolkit.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.environment.material.SurfaceShadingPoint;

/**
RenderPark's view of the self-emitted radiance distribution of a light
source material, with RenderPark's argument types: every function converts
its arguments and answers through the VITRAL PhongMaterial, whose emission
lobe is the model (plan step 2b).

Read-only after construction; does not own the PhongMaterial.
*/
public class PhongEmittanceDistributionFunction {
    private final PhongMaterial material;

    public PhongEmittanceDistributionFunction(PhongMaterial inMaterial) {
        material = inMaterial;
    }

    public static boolean edfIsTextured() {
        return false;
    }

    /**
    No emitter defines its own shading frame: callers build a default one
    */
    public static boolean edfShadingFrame(ShadingContext context, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    /** Emittance (self-emitted radiant exitance) [W / m^2] of the components in flags */
    public ColorRgb phongEmittance(ShadingContext context, int flags) {
        return material.emittance(flags);
    }

    /**
    Exitant radiance [W / m^2 sr] into the direction out; when
    probabilityDensityFunction is not null, the density of out under
    phongEdfSample goes there
    */
    public ColorRgb phongEdfEval(ShadingContext context, Vector3D out, int flags, double[] probabilityDensityFunction) {
        if ( context == null ) {
            return material.emissionEvaluate(null, MaterialBoundary.toVitral(out), flags, probabilityDensityFunction);
        }
        final SurfaceShadingPoint point = context.toShadingPoint();
        return material.emissionEvaluate(point, MaterialBoundary.toVitral(out), flags, probabilityDensityFunction);
    }

    /**
    Samples an emitted direction; the radiance along it (into
    selfEmittedRadiance, an out-parameter that is mutated) and its density
    go to the non-null arguments
    */
    public Vector3D phongEdfSample(
        ShadingContext context,
        int flags,
        double xi1,
        double xi2,
        ColorRgbMutable selfEmittedRadiance,
        double[] probabilityDensityFunction)
    {
        final ColorRgb[] radiance = selfEmittedRadiance != null ? new ColorRgb[1] : null;
        final Vector3D direction;
        if ( context == null ) {
            direction = MaterialBoundary.fromVitral(material.emissionSample(
                null, flags, xi1, xi2, radiance, probabilityDensityFunction));
        } else {
            final SurfaceShadingPoint point = context.toShadingPoint();
            direction = MaterialBoundary.fromVitral(material.emissionSample(
                point, flags, xi1, xi2, radiance, probabilityDensityFunction));
        }
        if ( selfEmittedRadiance != null ) {
            selfEmittedRadiance.set(radiance[0].getR(), radiance[0].getG(), radiance[0].getB());
        }
        return direction;
    }
}
