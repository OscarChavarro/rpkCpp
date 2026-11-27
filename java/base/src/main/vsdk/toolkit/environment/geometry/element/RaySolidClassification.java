package vsdk.toolkit.environment.geometry.element;

import java.util.ArrayList;

/**
For N ordered boundaries there are N+1 segment membership values. Endpoint
events are included; a zero-length endpoint segment has the limiting state
just outside the query interval. Point tangencies do not create solid segments.
On failure both arrays are empty. Clear retains storage for query/worker reuse.
Elements of `boundaries` are values: add copies (`new RayBoundary(b)`).
*/
public class RaySolidClassification {
    public final ArrayList<RayBoundary> boundaries;
    public final ArrayList<Boolean> insideSegments;

    public RaySolidClassification() { boundaries = new ArrayList<>(); insideSegments = new ArrayList<>(); }

    public void clear()
    {
        boundaries.clear();
        insideSegments.clear();
    }

    public RaySolidClassification(RaySolidClassification other)
    {
        this();
        for (int i = 0; i < other.boundaries.size(); i++) boundaries.add(new RayBoundary(other.boundaries.get(i)));
        insideSegments.addAll(other.insideSegments);
    }

    public RaySolidClassification assign(RaySolidClassification other)
    {
        // Reuses the existing storage, see RayInstancePath.
        if (this != other) {
            boundaries.clear();
            for (int i = 0; i < other.boundaries.size(); i++) boundaries.add(new RayBoundary(other.boundaries.get(i)));
            insideSegments.clear();
            for (int i = 0; i < other.insideSegments.size(); i++) insideSegments.add(other.insideSegments.get(i));
        }
        return this;
    }
}
