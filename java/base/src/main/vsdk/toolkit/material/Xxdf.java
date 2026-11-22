package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.material.ScatteringFrame;

/**
General definitions for edf, brdf, btdf, etc.
*/
public class Xxdf {
    private Xxdf() {
    }

    /**
    Ideal reflection and refraction directions, answered by VITRAL's
    ScatteringFrame (plan step 2b) on the same single precision values
    */
    public static Vector3D idealReflectedDirection(Vector3D in, Vector3D normal) {
        return MaterialBoundary.fromVitral(ScatteringFrame.idealReflection(
            MaterialBoundary.toVitral(in), MaterialBoundary.toVitral(normal)));
    }

    /**
    Sets totalInternalReflection[0] to true or false accordingly; real part of the
    indices only
    */
    public static Vector3D idealRefractedDirection(
        Vector3D in,
        Vector3D normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        boolean[] totalInternalReflection)
    {
        return MaterialBoundary.fromVitral(ScatteringFrame.idealRefraction(
            MaterialBoundary.toVitral(in), MaterialBoundary.toVitral(normal),
            inIndex.getNr(), outIndex.getNr(), totalInternalReflection));
    }
}
