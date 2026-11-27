package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.common.Entity;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;

/**
Identity is borrowed; transforms are owned snapshots from child to parent and
back.
*/
public class RayInstanceStep {
    public Entity instance;
    public Matrix4x4d childToParent;
    public Matrix4x4d parentToChild;

    public RayInstanceStep() { instance = null; childToParent = new Matrix4x4d(); parentToChild = new Matrix4x4d(); }
    public RayInstanceStep(Entity value, Matrix4x4d forward, Matrix4x4d inverse)
    { instance = value; childToParent = forward; parentToChild = inverse; }
    /** Copy constructor (cpp value copy). */
    public RayInstanceStep(RayInstanceStep other)
    { instance = other.instance; childToParent = other.childToParent; parentToChild = other.parentToChild; }
    /** cpp copy assignment. */
    public RayInstanceStep assign(RayInstanceStep other)
    {
        if (this != other) { instance = other.instance; childToParent = other.childToParent; parentToChild = other.parentToChild; }
        return this;
    }
}
