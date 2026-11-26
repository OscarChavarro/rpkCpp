#include "vsdk/toolkit/environment/geometry/element/RayInstanceStep.h"
RayInstanceStep::RayInstanceStep() : instance(nullptr), childToParent(), parentToChild() {}
RayInstanceStep::RayInstanceStep(const Entity* value, const Matrix4x4d& forward,
    const Matrix4x4d& inverse) : instance(value), childToParent(forward), parentToChild(inverse) {}
