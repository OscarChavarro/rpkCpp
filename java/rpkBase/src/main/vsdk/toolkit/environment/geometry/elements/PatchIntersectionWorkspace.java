package vsdk.toolkit.environment.geometry.elements;


/**
Scratch of Patch.intersect and of the (u, v) computations it does: the hit
records and (u, v) out-parameters that the C++ port keeps on the stack.

Java adaptation (the C++ port has no counterpart): reusing these objects
avoids billions of short-lived allocations per scene. Workspace: mutable,
owned by one thread of execution (a scene query workspace, a form factor
workspace, or Patch's own one for the single threaded callers) and passed
explicitly to Patch.intersect. Nothing it holds escapes a call.
*/
public final class PatchIntersectionWorkspace {
    final RpkRayHit hit = new RpkRayHit();
    final vsdk.toolkit.environment.geometry.element.PlanarFace.RayHit faceHit = new vsdk.toolkit.environment.geometry.element.PlanarFace.RayHit();
    final double[] u = new double[1];
    final double[] v = new double[1];

}
