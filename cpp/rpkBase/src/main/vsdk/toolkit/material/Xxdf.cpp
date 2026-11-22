/**
Ideal reflection and refraction directions, answered by VITRAL's
ScatteringFrame (plan step 2b) on the same single precision values
*/
#include "vsdk/toolkit/environment/material/ScatteringFrame.h"
#include "vsdk/toolkit/material/Xxdf.h"

Vector3Df
Xxdf::idealReflectedDirection(const Vector3Df *in, const Vector3Df *normal) {
    return ScatteringFrame::idealReflection(
        *in, *normal);
}

/**
Sets totalInternalReflection to true or false accordingly; real part of the
indices only
*/
Vector3Df
Xxdf::idealRefractedDirection(
    const Vector3Df *in,
    const Vector3Df *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    return ScatteringFrame::idealRefraction(
        *in, *normal,
        inIndex.getNr(), outIndex.getNr(), totalInternalReflection);
}
