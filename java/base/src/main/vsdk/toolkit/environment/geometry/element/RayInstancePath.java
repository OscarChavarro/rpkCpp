package vsdk.toolkit.environment.geometry.element;

import java.util.ArrayList;

/** Surface locator and instance transforms, from leaf outward. Identities are borrowed. */
public class RayInstancePath {
    private long surfaceIndex;
    private final ArrayList<RayInstanceStep> instances;

    public RayInstancePath() { surfaceIndex = -1; instances = new ArrayList<>(); }
    public RayInstancePath(RayInstancePath other)
    {
        surfaceIndex = other.surfaceIndex;
        instances = new ArrayList<>();
        for (int i = 0; i < other.instances.size(); i++) instances.add(new RayInstanceStep(other.instances.get(i)));
    }
    public RayInstancePath assign(RayInstancePath other)
    {
        // Reuses the existing storage: assignment of query results must not
        // allocate once a workspace has reached its working size.
        if (this != other) {
            surfaceIndex = other.surfaceIndex;
            instances.clear();
            for (int i = 0; i < other.instances.size(); i++) instances.add(new RayInstanceStep(other.instances.get(i)));
        }
        return this;
    }
    public boolean setSurfaceIndex(long index)
    {
        if (index < -1) return false;
        surfaceIndex = index; return true;
    }
    public long getSurfaceIndex() { return surfaceIndex; }
    public boolean appendInstance(RayInstanceStep step)
    { return step.instance != null && instances.add(new RayInstanceStep(step)); }
    public long getInstanceCount() { return instances.size(); }
    public boolean getInstance(long index, RayInstanceStep result)
    {
        if (index < 0 || index >= instances.size()) return false;
        result.assign(instances.get((int)index)); return true;
    }
    public void clearInstances() { instances.clear(); }
    public void clear() { surfaceIndex = -1; instances.clear(); }
}
