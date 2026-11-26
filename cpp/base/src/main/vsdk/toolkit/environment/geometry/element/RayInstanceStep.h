#ifndef __RAY_INSTANCE_STEP__
#define __RAY_INSTANCE_STEP__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
class Entity;

// Identity is borrowed; transforms are owned snapshots from child to parent and back.
class RayInstanceStep {
public:
    const Entity* instance;
    Matrix4x4d childToParent;
    Matrix4x4d parentToChild;
    RayInstanceStep();
    RayInstanceStep(const Entity* instance, const Matrix4x4d& childToParent,
        const Matrix4x4d& parentToChild);
};
#endif
