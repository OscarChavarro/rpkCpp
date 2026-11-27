package vsdk.toolkit.environment.scene;

import java.util.ArrayList;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayCrossing;
import vsdk.toolkit.environment.geometry.element.RayCrossingKind;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayInstanceStep;
import vsdk.toolkit.environment.material.Material;

/**
Buffers events so a failed source query never emits a partial result.
*/
public class InstanceRayCrossingBuffer implements RayCrossingSink {
    private final ArrayList<RayBoundary> boundaries = new ArrayList<RayBoundary>();

    private final ArrayList<RayCrossingKind> kinds = new ArrayList<RayCrossingKind>();

    private final ArrayList<SceneSolid> bodies = new ArrayList<SceneSolid>();

    private final ArrayList<Material> materials = new ArrayList<Material>();

    private final RayInstanceStep instance = new RayInstanceStep();

    private SceneSolid body;

    private Material material;

    private boolean failed;

    public InstanceRayCrossingBuffer(RayInstanceStep value, SceneSolid owner, Material appearance) {
        instance.assign(value);
        this.body = owner;
        this.material = appearance;
        this.failed = false;}

    @Override
    public void crossing(RayCrossing event) {
    if (failed) return;
    RayBoundary boundary = new RayBoundary(event);
    if (!boundary.appendInstance(instance) || !boundaries.add(boundary) || !kinds.add(event.kind) ||
        !bodies.add(event.body != null ? event.body : body) || !materials.add(event.material != null ? event.material : material))
        failed = true;
}

    public boolean isValid() { return !failed; }

    public void deliver(RayCrossingSink sink) {
    if (failed) return;
    for (long i=0; i<boundaries.size(); ++i) {
        RayCrossing event = new RayCrossing(boundaries.get((int)(i)).t, boundaries.get((int)(i)).geometry, kinds.get((int)(i)));
        event.assign(boundaries.get((int)(i))); // RayBoundary part (cpp static_cast<RayBoundary&>)
        event.body = bodies.get((int)(i)); event.material = materials.get((int)(i));
        sink.crossing(event);
    }
}
}
