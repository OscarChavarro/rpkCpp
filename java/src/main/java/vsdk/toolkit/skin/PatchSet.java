package vsdk.toolkit.skin;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;

public final class PatchSet extends Geometry {
    private ArrayList<Patch> patchList;
    private boolean memoryPoolManaged;

    public PatchSet(ArrayList<Patch> input) {
        super(GeometryClassId.PATCH_SET);
        memoryPoolManaged = false;
        patchList = new ArrayList<>();
        for (int i = 0; input != null && i < input.size(); i++) {
            patchList.add(input.get(i));
        }

        Geometry.patchListBounds(getPatchList(), boundingBox);
        boundingBox.enlargeTinyBit();
        bounded = true;
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
    public RayHit discretizationIntersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        if (!discretizationIntersectPreTest(ray, minimumDistance, maximumDistance)) {
            return null;
        }

        return Geometry.patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }

    public ArrayList<Patch> getPatchList() {
        return patchList;
    }

    public boolean isMemoryPoolManaged() {
        return memoryPoolManaged;
    }

    public void setMemoryPoolManaged(boolean value) {
        memoryPoolManaged = value;
    }
}
