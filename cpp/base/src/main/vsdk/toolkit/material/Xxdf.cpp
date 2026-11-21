/**
Ideal reflection and refraction directions, answered by VITRAL's
ScatteringFrame (plan step 2b) on the same single precision values
*/
#include "vsdk/toolkit/environment/material/ScatteringFrame.h"
#include "vsdk/toolkit/material/MaterialBoundary.h"
#include "vsdk/toolkit/material/Xxdf.h"

Vector3D
Xxdf::idealReflectedDirection(const Vector3D *in, const Vector3D *normal) {
    return MaterialBoundary::fromVitral(ScatteringFrame::idealReflection(
        MaterialBoundary::toVitral(*in), MaterialBoundary::toVitral(*normal)));
}

/**
Sets totalInternalReflection to true or false accordingly; real part of the
indices only
*/
Vector3D
Xxdf::idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    return MaterialBoundary::fromVitral(ScatteringFrame::idealRefraction(
        MaterialBoundary::toVitral(*in), MaterialBoundary::toVitral(*normal),
        inIndex.getNr(), outIndex.getNr(), totalInternalReflection));
}
