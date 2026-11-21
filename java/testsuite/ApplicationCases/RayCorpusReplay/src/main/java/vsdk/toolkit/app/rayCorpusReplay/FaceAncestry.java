package vsdk.toolkit.app.rayCorpusReplay;

import java.util.ArrayList;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;

/**
The geometries of the scene's cluster hierarchy by identifier, and for every
face the chain of geometries that contain it (its leaf geometry, then each
enclosing cluster up to the root). The legacy queries skip an excluded
geometry with everything below it; a face-level index applies the same rule
by rejecting a face when one of its ancestors is excluded.

Built once from the scene; read-only afterwards, shared by every worker.
*/
public final class FaceAncestry {
    private final ArrayList<Geometry> geometryById;
    private final ArrayList<Integer> parentById; // -1 at the root
    private final int[] leafByFace; // Identifier of the geometry holding each face, -1 if none

    public FaceAncestry(Scene scene) {
        geometryById = new ArrayList<>();
        parentById = new ArrayList<>();
        leafByFace = new int[scene.getFaceBounds().getNumberOfFaces()];
        for (int i = 0; i < leafByFace.length; i++) {
            leafByFace[i] = -1;
        }
        visit(scene.clusteredRootGeometry, -1);
    }

    private void visit(Geometry geometry, int parentId) {
        final int id = geometry.id;
        while (geometryById.size() <= id) {
            geometryById.add(null);
            parentById.add(-1);
        }
        geometryById.set(id, geometry);
        parentById.set(id, parentId);
        if (geometry.isCompound()) {
            ArrayList<Geometry> children = ((Compound)geometry).children;
            for (int i = 0; children != null && i < children.size(); i++) {
                visit(children.get(i), id);
            }
            return;
        }
        ArrayList<Patch> patches = Geometry.patchListReference(geometry);
        for (int i = 0; patches != null && i < patches.size(); i++) {
            final int face = patches.get(i).getFaceIndex();
            if (face >= 0 && leafByFace[face] < 0) {
                leafByFace[face] = id;
            }
        }
    }

    /** The geometry with this identifier, or null when the hierarchy has none */
    public Geometry getGeometry(int id) {
        return id >= 0 && id < geometryById.size() ? geometryById.get(id) : null;
    }

    /** True when one of the geometries containing the face is excluded */
    public boolean excludesFace(int faceIndex, RayExclusionSet exclusions) {
        if (exclusions.getExcludedGeometry1() == null && exclusions.getExcludedGeometry2() == null) {
            return false;
        }
        for (int id = leafByFace[faceIndex]; id >= 0; id = parentById.get(id)) {
            if (exclusions.excludesGeometry(geometryById.get(id))) {
                return true;
            }
        }
        return false;
    }
}
