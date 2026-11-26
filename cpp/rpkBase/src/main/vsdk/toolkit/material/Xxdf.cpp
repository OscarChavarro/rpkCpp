/**
Ideal reflection and refraction directions, answered by VITRAL's
ScatteringFrame (plan step 2b) on the same double precision values
*/
#include "vsdk/toolkit/environment/material/ScatteringFrame.h"
#include "vsdk/toolkit/material/Xxdf.h"

Vector3Dd
Xxdf::idealReflectedDirection(const Vector3Dd *in, const Vector3Dd *normal) {
    return ScatteringFrame::idealReflection(
        *in, *normal);
}

/**
Sets totalInternalReflection to true or false accordingly; real part of the
indices only
*/
Vector3Dd
Xxdf::idealRefractedDirection(
    const Vector3Dd *in,
    const Vector3Dd *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    return ScatteringFrame::idealRefraction(
        *in, *normal,
        inIndex.getNr(), outIndex.getNr(), totalInternalReflection);
}
