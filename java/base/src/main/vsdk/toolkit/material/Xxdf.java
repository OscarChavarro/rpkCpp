package vsdk.toolkit.material;

/**
General definitions for edf, brdf, btdf, etc.
*/

/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. [GLAS1989] An Introduction to Raytracing (Glassner)
*/

// Only real part of n for now

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class Xxdf {
    public static final float PHONG_LOWEST_SPECULAR_EXP = 250.0f;

    public static Vector3D idealReflectedDirection(Vector3D in, Vector3D normal) {
        double tmp = 2.0 * normal.dotProduct(in);
        Vector3D result = new Vector3D();

        result.scaledCopy((float)tmp, normal);
        result.subtraction(in, result);
        result.normalize(Numeric.EPSILON_FLOAT);

        return result;
    }

    public static Vector3D idealRefractedDirection(
        Vector3D in,
        Vector3D normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        boolean[] totalInternalReflection) {
        float refractionIndex = inIndex.getNr() / outIndex.getNr();
        float ci = -in.dotProduct(normal);
        float ct2 = 1.0f + refractionIndex * refractionIndex * (ci * ci - 1.0f);

        if (ct2 < 0.0f) {
            if (totalInternalReflection != null && totalInternalReflection.length > 0) {
                totalInternalReflection[0] = true;
            }
            return idealReflectedDirection(in, normal);
        }

        if (totalInternalReflection != null && totalInternalReflection.length > 0) {
            totalInternalReflection[0] = false;
        }

        float ct = (float)Math.sqrt(ct2);
        float normalScale = refractionIndex * ci - ct;

        Vector3D result = new Vector3D();
        result.scaledCopy(refractionIndex, in);
        result.sumScaled(result, normalScale, normal);
        result.normalize(Numeric.EPSILON_FLOAT);

        return result;
    }
}
