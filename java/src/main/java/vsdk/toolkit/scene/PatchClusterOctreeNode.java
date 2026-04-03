package vsdk.toolkit.scene;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.PatchSet;

/**
Clusters of patches. A hierarchy of clusters is automatically built after loading a scene.

Patch cluster hierarchies: after loading a scene, an octree hierarchy of patch
clusters is built. Since only patch position and size are relevant for
constructing this hierarchy (and not, for example, material properties or the
original geometry ownership), the generated structure is often more efficient
for ray traversal and clustering-based radiosity routines.

Reference:
- Per Christensen, PhD Thesis "Hierarchical Techniques for Glossy Global Illumination",
  Univ. of Washington, 1995, p116-117 - section 6.7 "Creation of Clusters"
*/
public class PatchClusterOctreeNode {
    private static final int XYZ_EQUAL_MASK = 0x08;
    // No clusters are created with less than this number of patches.
    private static final int MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER = 3;

    private ArrayList<Patch> patches;
    private PatchClusterOctreeNode[] children; // Clusters form an octree
    private BoundingBox boundingBox; // Bounding box for the cluster
    private Vector3D boundingBoxCentroid; // Mid-point of the bounding box
    private static ArrayList<Geometry> clusterNodeGeometriesToDelete;

    /**
    Creates an empty cluster with initialized bounding box
    */
    private PatchClusterOctreeNode() {
        boundingBox = new BoundingBox();
        boundingBoxCentroid = new Vector3D();
        boundingBoxCentroid.set(0.0f, 0.0f, 0.0f);
        patches = new ArrayList<>();

        children = new PatchClusterOctreeNode[8];
        for (int i = 0; i < 8; i++) {
            children[i] = null;
        }
    }

    /**
    Creates a toplevel cluster. The patch list of the cluster contains all inPatches.
    */
    public PatchClusterOctreeNode(ArrayList<Patch> inPatches) {
        boundingBox = new BoundingBox();
        boundingBoxCentroid = new Vector3D();
        boundingBoxCentroid.set(0.0f, 0.0f, 0.0f);

        children = new PatchClusterOctreeNode[8];
        for (int i = 0; i < 8; i++) {
            children[i] = null;
        }

        patches = new ArrayList<>();
        for (int i = 0; inPatches != null && i < inPatches.size(); i++) {
            clusterAddPatch(inPatches.get(i));
        }

        boundingBoxCentroid = boundingBox.center();
    }

    public void destroy() {
        // Can not delete the list since it is being transferred to geometry...
        for (int i = 0; i < 8; i++) {
            if (children[i] != null) {
                children[i].destroy();
                children[i] = null;
            }
        }

        // Containing just reference to patches owned by external objects
        patches = null;
    }

    private static void addToDeletionCache(Geometry geometry) {
        if (clusterNodeGeometriesToDelete == null) {
            clusterNodeGeometriesToDelete = new ArrayList<>();
        }
        clusterNodeGeometriesToDelete.add(geometry);
    }

    public static void deleteCachedGeometries() {
        if (clusterNodeGeometriesToDelete == null) {
            return;
        }
        for (int i = 0; i < clusterNodeGeometriesToDelete.size(); i++) {
            Geometry geometry = clusterNodeGeometriesToDelete.get(i);
            if (geometry != null) {
                geometry.isDuplicate = false;
                geometry.radianceData = null; // This was duplicated
                Geometry.destroy(geometry);
            }
        }
        clusterNodeGeometriesToDelete.clear();
        clusterNodeGeometriesToDelete = null;
    }

    /**
    Adds a patch to a cluster. The bounding box is enlarged if necessary, but
    the midpoint is not updated (it's more efficient to do that once, after all
    patches have been added to the cluster and the bounding box is fully
    determined)
    */
    private void clusterAddPatch(Patch patch) {
        if (patch != null) {
            patches.add(patch);

            BoundingBox patchBoundingBox = new BoundingBox();

            if (patch.boundingBox != null) {
                patchBoundingBox.copyFrom(patch.boundingBox);
            }
            else {
                patch.computeAndGetBoundingBox(patchBoundingBox);
            }
            boundingBox.enlarge(patchBoundingBox);
        }
    }

    private int octantIndex(Vector3D p) {
        Vector3D c = boundingBox.center();
        int index = 0;

        if (p.x > c.x) {
            index |= 1;
        }

        if (p.y > c.y) {
            index |= 2;
        }

        if (p.z > c.z) {
            index |= 4;
        }

        return index;
    }

    /**
    - Checks the size of patches[patchIndexOnParent] patch with reference to the bounding box of the cluster
    - If the size of the patch is more than half the size of the cluster, the method returns
    - If the patch is smaller than half the size of the cluster, the position of
      its centroid is tested with reference to the centroid of the cluster
    - If the centroids coincide, the routine returns
    - If not, patches[patchIndexOnParent] is moved to the patch list of a sub-cluster of cluster. Which sub-cluster
      depends on the position of the patch with reference to the centroid of cluster
    - Returns true if the patch was moved to the sub-cluster
    - Returns false if the patch was not moved
    - previousClusterNode is the patch list element previous patches[patchIndexOnParent] (chasing pointers!), needed to be
      able to efficiently remove patches[patchIndexOnParent] from the patch list of cluster
    */
    private boolean movePatchToSubOctantCluster(int patchIndexOnParent) {
        // All patches that were added to the top cluster, which is being split now,
        // have a bounding box computed for them
        Patch patch = patches.get(patchIndexOnParent);
        BoundingBox patchBoundingBox = patch.boundingBox;

        // If the patch is larger than an octant, don´t move current patch from parent to sub-cluster
        float smallestBoxDimension = 10.0f * Numeric.EPSILON_FLOAT;

        if ((patchBoundingBox.dx() > smallestBoxDimension && patchBoundingBox.dx() > boundingBox.dx() * 0.5f) ||
            (patchBoundingBox.dy() > smallestBoxDimension && patchBoundingBox.dy() > boundingBox.dy() * 0.5f) ||
            (patchBoundingBox.dz() > smallestBoxDimension && patchBoundingBox.dz() > boundingBox.dz() * 0.5f)) {
            return false;
        }

        // Check the position of the centroid of the bounding box of the patch with reference to the
        // centroid of the cluster
        Vector3D midPatch = patchBoundingBox.center();

        int selectedChildOctantIndex = octantIndex(midPatch);

        // If the centroids (almost by EPSILON) coincides, don´t move current patch from parent cluster to sub-cluster
        if (selectedChildOctantIndex == XYZ_EQUAL_MASK) {
            return false;
        }

        // Otherwise, move the patch to the sub cluster with index selectedChildOctantIndex
        PatchClusterOctreeNode selectedChildCluster = children[selectedChildOctantIndex];

        patches.remove(patchIndexOnParent);
        selectedChildCluster.patches.add(patch);

        // Enlarge the bounding box the of sub-cluster
        selectedChildCluster.boundingBox.enlarge(patchBoundingBox);

        // Current patch was moved to the sub-cluster
        return true;
    }

    /**
    Splits a cluster into sub-clusters: it is assumed that cluster is a cluster without
    sub-clusters yet. If cluster is nullptr or there are no more than MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER
    patches in the cluster, the routine simply returns. If there are more patches, 8
    sub-clusters are created for the cluster. Then for each patch, if the patch
    is smaller than an octant, the patch is moved to the octant containing its
    centroid. Otherwise, the patch remains a direct child of the cluster. Finally,
    sub-clusters with zero patches are disposed off and the procedure recursively repeated
    for each sub-cluster
    */
    public void splitCluster() {
        // Don't split the cluster if it contains too few patches
        if (patches != null && patches.size() <= MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER) {
            return;
        }

        // Create eight sub-clusters for the cluster with initialized bounding box
        for (int i = 0; i < 8; i++) {
            children[i] = new PatchClusterOctreeNode();
        }

        // Check and possibly move each of the patches in the cluster to a sub-cluster
        for (int i = 0; patches != null && i < patches.size(); i++) {
            if (movePatchToSubOctantCluster(i)) {
                i--;
            }
        }

        // Dispose of sub-clusters containing no patches, call splitCluster recursively for not empty sub-clusters
        for (int i = 0; i < 8; i++) {
            if (children[i].patches.size() == 0) {
                children[i] = null;
            }
            else {
                children[i].boundingBoxCentroid = children[i].boundingBox.center();
                children[i].splitCluster();
            }
        }
    }

    /**
    Converts a cluster hierarchy to a regular Geometry tree.
    This lets the standard ray traversal code operate on clusters, including
    operations such as shaft culling, without specialized traversal code.
    */
    public Geometry convertClusterToGeometry() {
        Geometry parentPatchesGeometry = null;
        if (patches != null && patches.size() > 0) {
            parentPatchesGeometry = new PatchSet(patches);
            addToDeletionCache(parentPatchesGeometry);
        }

        ArrayList<Geometry> patchesGeometryList = new ArrayList<>();

        // The patches in the cluster are the first to be tested for intersection with
        if (parentPatchesGeometry != null) {
            patchesGeometryList.add(parentPatchesGeometry);
        }

        for (int i = 0; i < 8; i++) {
            Geometry child = null;
            if (children[i] != null) {
                child = children[i].convertClusterToGeometry();
            }

            if (child != null) {
                patchesGeometryList.add(child);
            }
        }

        Geometry newGeometry = new Compound(patchesGeometryList);
        addToDeletionCache(newGeometry);
        return newGeometry;
    }

    public void print(int level) {
        switch (level) {
            case 0:
                System.out.printf("= PatchClusterOctreeNode ================================================================\n");
                break;
            case 1:
                System.out.printf("  - ");
                break;
            case 2:
                System.out.printf("    . ");
                break;
            default:
                System.out.printf("      (%d) ", level);
                for (int i = 3; i < level; i++) {
                    System.out.printf(" ");
                }
                System.out.printf("-> ");
                break;
        }
        System.out.printf("%d patches: ", patches.size());
        for (int i = 0; i < patches.size(); i++) {
            System.out.printf("[%d]", patches.get(i).id);
        }
        System.out.printf("\n");
        for (int i = 0; i < 8; i++) {
            if (children[i] != null) {
                children[i].print(level + 1);
            }
        }
    }
}
