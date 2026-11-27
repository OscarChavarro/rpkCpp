package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

import java.util.ArrayList;
import vsdk.toolkit.environment.geometry.element.Ray;

public final class PatchSet extends RpkGeometry {
    private ArrayList<Patch> patchList;
    private boolean memoryPoolManaged;

    public PatchSet(ArrayList<Patch> input) {
        super(GeometryClassId.PATCH_SET);
        memoryPoolManaged = false;
        patchList = new ArrayList<>();
        resetFrom(input);
    }

    /**
    patchBounds: the union of the patches' bounds, which the caller already
    has (for instance from the scene's FaceBoundsTable); the set is then
    bounded exactly as by the first constructor, without visiting vertices
    */
    public PatchSet(ArrayList<Patch> input, RpkAxisAlignedBoundingBox patchBounds) {
        super(GeometryClassId.PATCH_SET);
        memoryPoolManaged = false;
        patchList = new ArrayList<>();
        resetFrom(input, patchBounds);
    }

    @Override
    public void destroy() {
        if (patchList != null) {
            patchList = null;
        }
        super.destroy();
    }

    /**
    DiscretizationIntersect returns null if the ray does not hit the discretization
    of the object. If the ray hits the object, a hit record is returned containing
    information about the intersection point. See geometry.h for more explanation.

    Tests whether the Ray intersect the patches in the list. See geometry.h
    (GeomDiscretizationIntersect()) for more explanation.
    */
    @Override
    public RpkRayHit discretizationIntersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        if (!discretizationIntersectPreTest(ray, minimumDistance, maximumDistance, exclusions)) {
            return null;
        }

        return RpkGeometry.patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }

    public ArrayList<Patch> getPatchList() {
        return patchList;
    }

    public void resetFrom(ArrayList<Patch> input) {
        if (patchList == null) {
            patchList = new ArrayList<>();
        }
        else {
            patchList.clear();
        }
        for (int i = 0; input != null && i < input.size(); i++) {
            patchList.add(input.get(i));
        }

        RpkGeometry.patchListBounds(getPatchList(), boundingBox);
        boundingBox.enlargeTinyBit();
        bounded = true;
    }

    /**
    Pooled counterpart of the bounded constructor: reuses this set for a new
    patch list whose bounds the caller already has
    */
    public void resetFrom(ArrayList<Patch> input, RpkAxisAlignedBoundingBox patchBounds) {
        if (patchList == null) {
            patchList = new ArrayList<>();
        }
        else {
            patchList.clear();
        }
        for (int i = 0; input != null && i < input.size(); i++) {
            patchList.add(input.get(i));
        }

        // A pooled set gets a fresh box, as a newly constructed one has
        boundingBox = new RpkAxisAlignedBoundingBox();
        boundingBox.enlarge(patchBounds);
        boundingBox.enlargeTinyBit();
        bounded = true;
    }

    public boolean isMemoryPoolManaged() {
        return memoryPoolManaged;
    }

    public void setMemoryPoolManaged(boolean value) {
        memoryPoolManaged = value;
    }
}
