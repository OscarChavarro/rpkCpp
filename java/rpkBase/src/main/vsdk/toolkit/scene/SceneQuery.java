package vsdk.toolkit.scene;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahHierarchy;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.environment.geometry.spatialIndex.BinnedSahSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.GridSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.LinearSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialGroupTree;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryMode;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryResult;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;

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
    private VoxelGrid legacyGrid; // Owned; LEGACY_VOXEL_ACCELERATION only
    private SpatialIndex index; // Owned; the other methods

    private static void boxValues(BoundingBox box, double[] values) {
        values[0] = box.minX();
        values[1] = box.minY();
        values[2] = box.minZ();
        values[3] = box.maxX();
        values[4] = box.maxY();
        values[5] = box.maxZ();
    }

    /**
    Adds the members of a geometry of the cluster hierarchy to its group, in the
    order the legacy grid visits them: the children of a compound, the patches
    (as their face indices) of any other geometry
    */
    private static void addMembers(SpatialGroupTree tree, int group, Geometry geometry) {
        if (geometry.isCompound()) {
            ArrayList<Geometry> children = ((Compound)geometry).children;
            for (int i = 0; children != null && i < children.size(); i++) {
                Geometry child = children.get(i);
                double[] box = new double[6];
                boxValues(child.boundingBox, box);
                addMembers(tree, tree.addGroup(group, child.bounded ? box : null), child);
            }
            return;
        }
        ArrayList<Patch> patches = Geometry.patchListReference(geometry);
        for (int i = 0; patches != null && i < patches.size(); i++) {
            tree.addPrimitive(group, patches.get(i).getFaceIndex());
        }
    }

    public SceneQuery(Scene inScene, SceneAccelerationMethod inMethod) {
        scene = inScene;
        method = inMethod;
        legacyGrid = null;
        index = null;
        if (method == SceneAccelerationMethod.LEGACY_VOXEL_ACCELERATION) {
            legacyGrid = new VoxelGrid(scene.clusteredRootGeometry);
        }
        else {
            index = buildIndex(scene, method);
        }
    }

    public void destroy() {
        if (legacyGrid != null) {
            legacyGrid.freeVoxelGridElements();
            legacyGrid = null;
        }
        index = null;
    }

    /**
    The VITRAL index of a method over the scene's face table, primitive i
    being face i, or null for LEGACY_VOXEL_ACCELERATION. The caller owns it.
    WALD2007 gets the face boxes padded by the face tolerance plus a relative
    margin, so that each contains every point the face test may accept; the
    grid gets the unpadded boxes (it pads them as the legacy grid does) and
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
                BoundingBox box = faceBounds.get(i);
                final double extent = box.maxExtent();
                low[0] = box.minX();
                low[1] = box.minY();
                low[2] = box.minZ();
                high[0] = box.maxX();
                high[1] = box.maxY();
                high[2] = box.maxZ();
                final double tolerance = scene.getFace(i).tolerance;
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
            Geometry root = scene.clusteredRootGeometry;
            double[] rootBox = new double[6];
            boxValues(root.boundingBox, rootBox);
            SpatialGroupTree tree = new SpatialGroupTree(rawBoxes, rootBox);
            addMembers(tree, 0, root);
            return new GridSpatialIndex(tree);
        }

        return null;
    }

    /** The option name of a method: legacy, voxel, wald2007, bruteforce */
    public static String methodName(SceneAccelerationMethod method) {
        switch (method) {
            case LEGACY_VOXEL_ACCELERATION:
                return "legacy";
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
            SceneAccelerationMethod.LEGACY_VOXEL_ACCELERATION,
            SceneAccelerationMethod.VOXEL_ACCELERATION,
            SceneAccelerationMethod.WALD2007_ACCELERATION,
            SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION
        };
        for (int i = 0; i < 4; i++) {
            if (name.equals(methodName(methods[i]))) {
                return methods[i];
            }
        }
        return null;
    }

    public SceneAccelerationMethod getMethod() {
        return method;
    }

    public VoxelGrid getLegacyGrid() {
        return legacyGrid;
    }

    public SpatialIndex getIndex() {
        return index;
    }

    /**
    As RenderPark's VoxelGrid.gridIntersect: returns hitStore filled with the
    accepted hit and sets maximumDistance[0] to its distance, or returns null
    and leaves both unchanged when no face is hit in
    [minimumDistance, maximumDistance[0]]
    */
    public RayHit intersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore,
        RayExclusionSet exclusions,
        SceneQueryWorkspace workspace) {
        if (legacyGrid != null) {
            return legacyGrid.gridIntersect(
                ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions, workspace.getLegacyWorkspace());
        }

        SceneFaceRayTest test = workspace.getFaceRayTest(scene);
        test.set(ray, hitFlags, exclusions, hitStore);
        final vsdk.toolkit.environment.geometry.element.Ray indexRay =
            vsdk.toolkit.environment.geometry.element.Ray.fromAffine(
                new Vector3Dd(ray.position.x(), ray.position.y(), ray.position.z()),
                new Vector3Dd(ray.direction.x(), ray.direction.y(), ray.direction.z()),
                0.0);
        final SpatialQueryMode mode = (hitFlags & RayHitFlag.ANY) != 0
            ? SpatialQueryMode.ANY_HIT : SpatialQueryMode.CLOSEST_HIT;
        final SpatialQueryResult result = index.query(
            indexRay, minimumDistance, maximumDistance[0], mode, test, workspace.getIndexWorkspace());
        if (!result.hasHit()) {
            return null;
        }
        maximumDistance[0] = (float)result.getParameter();
        return hitStore;
    }
}
