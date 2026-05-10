package vsdk.toolkit.skin;

import java.util.ArrayList;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.skin.RayHitFlag;

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
public class Geometry {
    public static int nextGeometryId = 0;
    public static Geometry excludedGeometry1 = null;
    public static Geometry excludedGeometry2 = null;

    public int id; // Unique ID number
    public BoundingBox boundingBox;
    public MinMaxBox rayIntersectionBox;
    public Element radianceData; // Data specific to the radiance algorithm being used
    public int itemCount;
    public boolean bounded; // A flag indicating if the geometry has a bounding box, non-zero if bounded geometry
    public boolean shaftCullGeometry; // Generated during shaft culling
    public boolean omit; // Indicates that the Geometry should not be considered for some tasks.
    public boolean isDuplicate;

    public int className;

    public Geometry() {
        id = 0;
        boundingBox = new BoundingBox();
        rayIntersectionBox = null;
        radianceData = null;
        itemCount = 0;
        bounded = false;
        omit = false;
        isDuplicate = false;
        className = GeometryClassId.UNDEFINED;
        shaftCullGeometry = false;
    }

    /**
    This function is used to create a new geometry with given specific data and
    methods.
    */
    public Geometry(int inClassName) {
        Statistics.instance().reader.numberOfGeometries++;
        id = nextGeometryId;
        nextGeometryId++;
        className = inClassName;
        isDuplicate = false;
        bounded = false;
        shaftCullGeometry = false;
        rayIntersectionBox = null;
        radianceData = null;
        itemCount = 0;
        omit = false;
        boundingBox = new BoundingBox();
    }

    public void destroy() {
        rayIntersectionBox = null;
        if (radianceData != null && !isDuplicate) {
            radianceData = null;
        }
    }

    /**
    This function returns a bounding box for the geometry.
    */
    public BoundingBox getBoundingBox() {
        return boundingBox;
    }

    public MinMaxBox getRayIntersectionBox() {
        if (rayIntersectionBox == null) {
            rayIntersectionBox = new MinMaxBox(boundingBox);
        }
        else {
            rayIntersectionBox.updateFromBoundingBox(boundingBox);
        }
        return rayIntersectionBox;
    }

    /**
    This function destroys the given geometry.
    */
    public static void destroy(Geometry geometry) {
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
    possible, e.g. CSG objects. If the given geometry is a primitive, zero is
    returned. A primitive geometry is a geometry that does not consist of
    simpler geometries.
    */
    public boolean isCompound() {
        return className == GeometryClassId.COMPOUND;
    }

    private static ArrayList<Geometry> cloneGeometryList(ArrayList<Geometry> input) {
        ArrayList<Geometry> output = new ArrayList<>();
        for (int i = 0; input != null && i < input.size(); i++) {
            output.add(input.get(i));
        }
        return output;
    }

    /**
    Returns a list of the simpler geometries making up an aggregate geometry.
    A null pointer is returned if the geometry is a primitive.
    */
    public static ArrayList<Geometry> primitiveListCopy(Geometry geometry) {
        if (geometry.isCompound()) {
            return cloneGeometryList(((Compound)geometry).children);
        }
        return null;
    }

    public static ArrayList<Patch> patchListReference(Geometry geometry) {
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
    public Geometry clone() {
        if (className != GeometryClassId.PATCH_SET) {
            Logger.fatal(666, "duplicateIfPatchSet", "this should not happen");
        }

        PatchSet newPatchSet = new PatchSet(Geometry.patchListReference(this));
        newPatchSet.id = Statistics.instance().reader.numberOfGeometries;
        newPatchSet.boundingBox = boundingBox;
        newPatchSet.radianceData = radianceData;
        newPatchSet.itemCount = itemCount;
        newPatchSet.bounded = bounded;
        newPatchSet.shaftCullGeometry = shaftCullGeometry;
        newPatchSet.omit = omit;
        newPatchSet.className = className;
        newPatchSet.isDuplicate = true;

        Statistics.instance().reader.numberOfGeometries++;

        return newPatchSet;
    }

    /**
    Will avoid intersection testing with geom1 and geom2 (possibly null
    pointers). Can be used for avoiding immediate self-intersections.
    */
    public static void dontIntersect(Geometry geometry1, Geometry geometry2) {
        excludedGeometry1 = geometry1;
        excludedGeometry2 = geometry2;
    }

    public boolean discretizationIntersectPreTest(Ray ray, float minimumDistance, float[] maximumDistance) {
        if (this == excludedGeometry1 || this == excludedGeometry2) {
            return false;
        }

        if (bounded) {
            Vector3D vTmp = new Vector3D();

            // Check ray/bounding volume intersection
            vTmp.sumScaled(ray.position, minimumDistance, ray.direction);
            if (boundingBox.outOfBounds(vTmp)) {
                float[] nMaximumDistance = new float[] {maximumDistance[0]};
                MinMaxBox minMaxBox = getRayIntersectionBox();
                if (!minMaxBox.intersect(ray, minimumDistance, nMaximumDistance)) {
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
    public RayHit discretizationIntersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        if (!discretizationIntersectPreTest(ray, minimumDistance, maximumDistance)) {
            return null;
        }

        if (className == GeometryClassId.SURFACE_MESH) {
            return ((MeshSurface)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        else if (className == GeometryClassId.COMPOUND) {
            return ((Compound)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        else if (className == GeometryClassId.PATCH_SET) {
            return ((PatchSet)this).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        return null;
    }

    public static RayHit listDiscretizationIntersect(
        ArrayList<Geometry> geometryList,
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        RayHit hit = null;

        for (int i = 0; geometryList != null && i < geometryList.size(); i++) {
            RayHit h = geometryList.get(i).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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
    public static void listBounds(ArrayList<Geometry> geometryList, BoundingBox boundingBox) {
        for (int i = 0; geometryList != null && i < geometryList.size(); i++) {
            boundingBox.enlarge(geometryList.get(i).boundingBox);
        }
    }

    /**
    Tests whether the Ray intersect the patches in the list.
    */
    public static RayHit patchListIntersect(
        ArrayList<Patch> patchList,
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        RayHit hit = null;
        for (int i = 0; patchList != null && i < patchList.size(); i++) {
            RayHit h = patchList.get(i).intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
            if (h != null) {
                if ((hitFlags & RayHitFlag.ANY) != 0) {
                    return h;
                }
                hit = h;
            }
        }
        return hit;
    }

    protected static BoundingBox patchListBounds(ArrayList<Patch> patchList, BoundingBox boundingBox) {
        BoundingBox currentPatchBoundingBox = new BoundingBox();

        for (int i = 0; patchList != null && i < patchList.size(); i++) {
            patchList.get(i).computeAndGetBoundingBox(currentPatchBoundingBox);
            boundingBox.enlarge(currentPatchBoundingBox);
        }

        return boundingBox;
    }

    public boolean isExcluded() {
        return this == excludedGeometry1 || this == excludedGeometry2;
    }
}
