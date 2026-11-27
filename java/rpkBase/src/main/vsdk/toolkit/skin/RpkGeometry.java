package vsdk.toolkit.skin;

import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.PatchSet;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

import java.util.ArrayList;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;

/**
Currently, there are three types of geometries:
- Compound: an aggregate geometry which is basically a list of other
  geometries, useful for representing the scene in a hierarchical manner
- MeshSurface: a primitive geometry which is a list of patches representing
  an object with given Material properties
- PatchSet: a primitive geometry consisting of a list of patches

Each of these primitives has certain specific data. The geometry class
contains data that is independent of geometry type.
*/
public class RpkGeometry {
    public static int nextGeometryId = 0;

    // Interval [firstFaceIndex, lastFaceIndex] of Patch.getFaceIndex()
    // values of the patches below this geometry. Scene.buildFaceTable()
    // numbers the faces depth-first over the cluster hierarchy, so every
    // geometry of that hierarchy covers a contiguous interval. Empty
    // (first > last) until the face table is built.
    private int firstFaceIndex;
    private int lastFaceIndex;

    public int id; // Unique ID number
    public RpkAxisAlignedBoundingBox boundingBox;
    public Element radianceData; // Data specific to the radiance algorithm being used
    public int itemCount;
    public boolean bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    public boolean shaftCullGeometry; // Generated during shaft culling
    public boolean isDuplicate;

    public int className;

    public RpkGeometry() {
        firstFaceIndex = 0;
        lastFaceIndex = -1;
        id = 0;
        boundingBox = new RpkAxisAlignedBoundingBox();
        radianceData = null;
        itemCount = 0;
        bounded = false;
        isDuplicate = false;
        className = GeometryClassId.UNDEFINED;
        shaftCullGeometry = false;
    }

    /**
    This function is used to create a new geometry with given specific data and
    methods.
    */
    public RpkGeometry(int inClassName) {
        firstFaceIndex = 0;
        lastFaceIndex = -1;
        Statistics.instance().reader.numberOfGeometries++;
        id = nextGeometryId;
        nextGeometryId++;
        className = inClassName;
        isDuplicate = false;
        bounded = false;
        shaftCullGeometry = false;
        radianceData = null;
        itemCount = 0;
        boundingBox = new RpkAxisAlignedBoundingBox();
    }

    public void destroy() {
        if (radianceData != null && !isDuplicate) {
            radianceData = null;
        }
    }

    /**
    This function returns a bounding box for the geometry.
    */
    public RpkAxisAlignedBoundingBox getBoundingBox() {
        return boundingBox;
    }

    /**
    This function destroys the given geometry.
    */
    public static void destroy(RpkGeometry geometry) {
        if (geometry == null) {
            return;
        }
        geometry.destroy();
        Statistics.instance().reader.numberOfGeometries--;
    }

    /**
    This function returns nonzero if the given geometry is an aggregate. An
    aggregate is a geometry that consists of simpler geometries. Currently,
    there is only one type of aggregate geometry: the compound, which is basically
    just a list of simpler geometries. Other aggregate geometries are also
    possible, e.getG(). CSG objects. If the given geometry is a primitive, zero is
    returned. A primitive geometry is a geometry that does not consist of
    simpler geometries.
    */
    public boolean isCompound() {
        return className == GeometryClassId.COMPOUND;
    }

    public int getFirstFaceIndex() {
        return firstFaceIndex;
    }

    public int getLastFaceIndex() {
        return lastFaceIndex;
    }

    /**
    Records the interval of face indices covered by this geometry. Called by
    Scene.buildFaceTable() only; an empty interval is first > last
    */
    public void setFaceIndexRange(int first, int last) {
        firstFaceIndex = first;
        lastFaceIndex = last;
    }

    private static ArrayList<RpkGeometry> cloneGeometryList(ArrayList<RpkGeometry> input) {
        ArrayList<RpkGeometry> output = new ArrayList<>();
        for (int i = 0; input != null && i < input.size(); i++) {
            output.add(input.get(i));
        }
        return output;
    }

    /**
    Returns a list of the simpler geometries making up an aggregate geometry.
    A null pointer is returned if the geometry is a primitive.
    */
    public static ArrayList<RpkGeometry> primitiveListCopy(RpkGeometry geometry) {
        if (geometry.isCompound()) {
            return cloneGeometryList(((Compound)geometry).children);
        }
        return null;
    }

    public static ArrayList<Patch> patchListReference(RpkGeometry geometry) {
        if (geometry.className == GeometryClassId.SURFACE_MESH) {
            return ((MeshSurface)geometry).faces;
        }
        else if (geometry.className == GeometryClassId.PATCH_SET) {
            return ((PatchSet)geometry).getPatchList();
        }
        else if (geometry.className == GeometryClassId.COMPOUND) {
            return null;
        }
        return null;
    }

    /**
    This routine creates and returns a duplicate of the given geometry. Needed for
    shaft culling.
    */
    public RpkGeometry clone() {
        if (className != GeometryClassId.PATCH_SET) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "duplicateIfPatchSet", "this should not happen");
        }

        PatchSet newPatchSet = new PatchSet(RpkGeometry.patchListReference(this));
        newPatchSet.id = Statistics.instance().reader.numberOfGeometries;
        newPatchSet.boundingBox = boundingBox;
        newPatchSet.radianceData = radianceData;
        newPatchSet.itemCount = itemCount;
        newPatchSet.bounded = bounded;
        newPatchSet.shaftCullGeometry = shaftCullGeometry;
        newPatchSet.className = className;
        newPatchSet.isDuplicate = true;
        newPatchSet.setFaceIndexRange(firstFaceIndex, lastFaceIndex);

        Statistics.instance().reader.numberOfGeometries++;

        return newPatchSet;
    }

    // Scratch storage reused across pre-test calls (this method runs billions
    // of times per scene on the single threaded rendering core; the C++ port
    // keeps these on the stack)
    private static final double[] preTestScratchDistance = new double[1];

    public boolean discretizationIntersectPreTest(Ray ray, double minimumDistance, double[] maximumDistance, RayExclusionSet exclusions) {
        if (exclusions.excludesGeometry(this)) {
            return false;
        }

        if (bounded) {
            // Check ray/bounding volume intersection
            final Vector3Dd vTmp = ray.getOrigin().add(ray.getDirection().multiply(minimumDistance));
            if (boundingBox.outOfBounds(vTmp)) {
                double[] nMaximumDistance = preTestScratchDistance;
                nMaximumDistance[0] = maximumDistance[0];
                if (!boundingBox.intersect(ray, minimumDistance, nMaximumDistance)) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
    This routine returns null if the ray does not hit the discretization of the
    geometry. If the ray hits the discretization of the Geometry, containing
    (among other information) the hit patch is returned.
    */
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

        if (className == GeometryClassId.SURFACE_MESH) {
            return ((MeshSurface)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
        }
        else if (className == GeometryClassId.COMPOUND) {
            return ((Compound)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
        }
        else if (className == GeometryClassId.PATCH_SET) {
            return ((PatchSet)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
        }
        return null;
    }

    public static RpkRayHit listDiscretizationIntersect(
        ArrayList<RpkGeometry> geometryList,
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        RpkRayHit hit = null;

        for (int i = 0; geometryList != null && i < geometryList.size(); i++) {
            RpkRayHit h = geometryList.get(i).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
            if (h != null) {
                if ((hitFlags & RayHitFlag.ANY) != 0) {
                    return h;
                }
                hit = h;
            }
        }
        return hit;
    }

    /**
    This function computes a bounding box for a list of geometries.
    */
    public static void listBounds(ArrayList<RpkGeometry> geometryList, RpkAxisAlignedBoundingBox boundingBox) {
        for (int i = 0; geometryList != null && i < geometryList.size(); i++) {
            boundingBox.enlarge(geometryList.get(i).boundingBox);
        }
    }

    /**
    Tests whether the Ray intersect the patches in the list.
    */
    public static RpkRayHit patchListIntersect(
        ArrayList<Patch> patchList,
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        RpkRayHit hit = null;
        for (int i = 0; patchList != null && i < patchList.size(); i++) {
            RpkRayHit h = patchList.get(i).intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
            if (h != null) {
                if ((hitFlags & RayHitFlag.ANY) != 0) {
                    return h;
                }
                hit = h;
            }
        }
        return hit;
    }

    protected static RpkAxisAlignedBoundingBox patchListBounds(ArrayList<Patch> patchList, RpkAxisAlignedBoundingBox boundingBox) {
        RpkAxisAlignedBoundingBox currentPatchBoundingBox = new RpkAxisAlignedBoundingBox();

        for (int i = 0; patchList != null && i < patchList.size(); i++) {
            patchList.get(i).computeBounds(currentPatchBoundingBox);
            boundingBox.enlarge(currentPatchBoundingBox);
        }

        return boundingBox;
    }
}
