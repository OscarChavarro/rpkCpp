package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.material.ScatteringFrame;

/**
General definitions for edf, brdf, btdf, etc.
*/
public class Xxdf {
    private Xxdf() {
    }

    /**
    Ideal reflection and refraction directions, answered by VITRAL's
    ScatteringFrame (plan step 2b) on the same double precision values
    */
    public static Vector3Dd idealReflectedDirection(Vector3Dd in, Vector3Dd normal) {
        return ScatteringFrame.idealReflection(
            in, normal);
    }

    /**
    Sets totalInternalReflection[0] to true or false accordingly; real part of the
    indices only
    */
    public static Vector3Dd idealRefractedDirection(
        Vector3Dd in,
        Vector3Dd normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        boolean[] totalInternalReflection)
    {
        return ScatteringFrame.idealRefraction(
            in, normal,
            inIndex.getNr(), outIndex.getNr(), totalInternalReflection);
    }
}
