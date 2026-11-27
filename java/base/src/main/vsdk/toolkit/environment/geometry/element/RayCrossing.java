package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.scene.SceneSolid;
import vsdk.toolkit.environment.material.Material;

/** Leaf identity and optional instance/material attribution; references are borrowed. */
public class RayCrossing extends RayBoundary {
    public RayCrossingKind kind;
    public SceneSolid body;
    public Material material;

    public RayCrossing(double t, Geometry geometry, RayCrossingKind kind)
    { super(t, geometry); this.kind = kind; body = null; material = null; }
    public RayCrossing(RayCrossing other)
    { super(other); kind = other.kind; body = other.body; material = other.material; }
    public RayCrossing assign(RayCrossing other)
    {
        if (this != other) { super.assign(other); kind = other.kind; body = other.body; material = other.material; }
        return this;
    }
}
