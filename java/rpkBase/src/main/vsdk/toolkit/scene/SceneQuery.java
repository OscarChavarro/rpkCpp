package vsdk.toolkit.scene;

import java.util.ArrayList;

import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahHierarchy;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.environment.geometry.spatialIndex.BinnedSahSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.GridSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.LinearSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialGroupTree;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryMode;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryResult;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.RpkGeometry;

/**
The scene's visibility query: the nearest (or, with RayHitFlag.ANY, some)
face that a ray hits within an interval, through the spatial index chosen by
a SceneAccelerationMethod. Faces are the entries of the scene's face table;
the exact face test, flags and exclusions are RenderPark's Patch.intersect
for every method, so the methods differ only in which faces they test.
An ANY query may return another blocker with each method; its visibility
answer is the same.

Shared, read-only after construction: intersect writes only to the hit
record and the workspace it is given. Built after the face table; a change
of the scene geometry requires a new query.
*/
public final class SceneQuery {
    private final Scene scene;
    private final SceneAccelerationMethod method;
    private SpatialIndex index; // Owned

    private static void boxValues(RpkAxisAlignedBoundingBox box, double[] values) {
        values[0] = box.minX();
        values[1] = box.minY();
        values[2] = box.minZ();
        values[3] = box.maxX();
        values[4] = box.maxY();
        values[5] = box.maxZ();
    }

    /**
    Adds the members of a geometry of the cluster hierarchy to its group, in the
    order RenderPark's original grid visited them: the children of a compound, the patches
    (as their face indices) of any other geometry
    */
    private static void addMembers(SpatialGroupTree tree, int group, RpkGeometry geometry) {
        if (geometry.isCompound()) {
            ArrayList<RpkGeometry> children = ((Compound)geometry).children;
            for (int i = 0; children != null && i < children.size(); i++) {
                RpkGeometry child = children.get(i);
                double[] box = new double[6];
                boxValues(child.boundingBox, box);
                addMembers(tree, tree.addGroup(group, child.bounded ? box : null), child);
            }
            return;
        }
        ArrayList<Patch> patches = RpkGeometry.patchListReference(geometry);
        for (int i = 0; patches != null && i < patches.size(); i++) {
            tree.addPrimitive(group, patches.get(i).getFaceIndex());
        }
    }

    public SceneQuery(Scene inScene, SceneAccelerationMethod inMethod) {
        scene = inScene;
        method = inMethod;
        index = buildIndex(scene, method);
    }

    public void destroy() {
        index = null;
    }

    /**
    The VITRAL index of a method over the scene's face table, primitive i
    being face i. The caller owns it.
    WALD2007 gets the face boxes padded by the face tolerance plus a relative
    margin, so that each contains every point the face test may accept; the
    grid gets the unpadded boxes (it pads them as RenderPark's grid did) and
    the cluster hierarchy as its tree of groups.
    */
    public static SpatialIndex buildIndex(Scene scene, SceneAccelerationMethod method) {
        FaceBoundsTable faceBounds = scene.getFaceBounds();
        final int numberOfFaces = faceBounds.getNumberOfFaces();

        if (method == SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION) {
            return new LinearSpatialIndex(numberOfFaces);
        }

        if (method == SceneAccelerationMethod.WALD2007_ACCELERATION) {
            ArrayList<Double> boxes = new ArrayList<>(6 * numberOfFaces);
            double[] low = new double[3];
            double[] high = new double[3];
            for (int i = 0; i < numberOfFaces; i++) {
                RpkAxisAlignedBoundingBox box = faceBounds.get(i);
                final double extent = box.maxExtent();
                low[0] = box.minX();
                low[1] = box.minY();
                low[2] = box.minZ();
                high[0] = box.maxX();
                high[1] = box.maxY();
                high[2] = box.maxZ();
                final double tolerance = scene.getFace(i).getTolerance();
                for (int axis = 0; axis < 3; axis++) {
                    final double magnitude = Math.max(Math.abs(low[axis]), Math.abs(high[axis]));
                    final double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                    boxes.add(low[axis] - padding);
                }
                for (int axis = 0; axis < 3; axis++) {
                    final double magnitude = Math.max(Math.abs(low[axis]), Math.abs(high[axis]));
                    final double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                    boxes.add(high[axis] + padding);
                }
            }
            return new BinnedSahSpatialIndex(boxes, new ArrayList<>(), BinnedSahHierarchy.DEFAULT_MAXIMUM_LEAF_SIZE);
        }

        if (method == SceneAccelerationMethod.VOXEL_ACCELERATION) {
            ArrayList<Double> rawBoxes = new ArrayList<>(6 * numberOfFaces);
            double[] box = new double[6];
            for (int i = 0; i < numberOfFaces; i++) {
                boxValues(faceBounds.get(i), box);
                for (int k = 0; k < 6; k++) {
                    rawBoxes.add(box[k]);
                }
            }
            RpkGeometry root = scene.clusteredRootGeometry;
            double[] rootBox = new double[6];
            boxValues(root.boundingBox, rootBox);
            SpatialGroupTree tree = new SpatialGroupTree(rawBoxes, rootBox);
            addMembers(tree, 0, root);
            return new GridSpatialIndex(tree);
        }

        return null;
    }

    /** The option name of a method: voxel, wald2007, bruteforce */
    public static String methodName(SceneAccelerationMethod method) {
        switch (method) {
            case VOXEL_ACCELERATION:
                return "voxel";
            case WALD2007_ACCELERATION:
                return "wald2007";
            case BRUTE_FORCE_ACCELERATION:
                return "bruteforce";
            default:
                return "unknown";
        }
    }

    /** The method of an option name; null when the name is none of them */
    public static SceneAccelerationMethod methodFromName(String name) {
        if (name == null) {
            return null;
        }
        SceneAccelerationMethod[] methods = new SceneAccelerationMethod[] {
            SceneAccelerationMethod.VOXEL_ACCELERATION,
            SceneAccelerationMethod.WALD2007_ACCELERATION,
            SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION
        };
        for (int i = 0; i < 3; i++) {
            if (name.equals(methodName(methods[i]))) {
                return methods[i];
            }
        }
        return null;
    }

    public SceneAccelerationMethod getMethod() {
        return method;
    }

    public SpatialIndex getIndex() {
        return index;
    }

    /**
    As RenderPark's original grid query: returns hitStore filled with the
    accepted hit and sets maximumDistance[0] to its distance, or returns null
    and leaves both unchanged when no face is hit in
    [minimumDistance, maximumDistance[0]]
    */
    public RpkRayHit intersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions,
        SceneQueryWorkspace workspace) {
        SceneFaceRayTest test = workspace.getFaceRayTest(scene);
        test.set(ray, hitFlags, exclusions, hitStore);
        final vsdk.toolkit.environment.geometry.element.Ray indexRay =
            vsdk.toolkit.environment.geometry.element.Ray.fromAffine(
                new Vector3Dd(ray.getOrigin().x(), ray.getOrigin().y(), ray.getOrigin().z()),
                new Vector3Dd(ray.getDirection().x(), ray.getDirection().y(), ray.getDirection().z()),
                0.0);
        final SpatialQueryMode mode = (hitFlags & RayHitFlag.ANY) != 0
            ? SpatialQueryMode.ANY_HIT : SpatialQueryMode.CLOSEST_HIT;
        final SpatialQueryResult result = index.query(
            indexRay, minimumDistance, maximumDistance[0], mode, test, workspace.getIndexWorkspace());
        if (!result.hasHit()) {
            return null;
        }
        maximumDistance[0] = (double)result.getParameter();
        return hitStore;
    }
}
