package vsdk.toolkit.scene;

import java.util.ArrayList;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.PatchSet;

public class Scene {
    private static final String compoundType = "Compound";
    private static final String meshSurfaceType = "MeshSurface";
    private static final String patchSetType = "PatchSet";
    private static final String unknownType = "<unknown>";

    // Face table: every patch of the scene, in depth-first order of
    // clusteredRootGeometry; faceTable[i].getFaceIndex() == i
    private Patch[] faceTable;
    private int numberOfFaces;
    private FaceBoundsTable faceBounds; // Owned; bounds of the faceTable entries
    private SceneAccelerationMethod accelerationMethod; // Index of sceneQuery; WALD2007 unless an option says otherwise
    private SceneQuery sceneQuery; // Owned; built by buildSceneQuery
    private SceneQueryWorkspace sceneQueryWorkspace; // Owned; query scratch of the single-threaded callers

    public Background background;
    public Camera camera;
    public ArrayList<Geometry> geometryList;
    public ArrayList<Geometry> clusteredGeometryList;
    public Geometry clusteredRootGeometry;
    public VisibilityQueryRecorder visibilityQueryRecorder; // Ray capture runs only, otherwise null

    // The list of all patches in the current scene
    public ArrayList<Patch> patchList;

    // The light of all patches on light sources, useful for e.getG(). next event estimation in Monte Carlo raytracing etc.
    public ArrayList<Patch> lightSourcePatchList;

    public Scene() {
        background = null;
        camera = new Camera();
        geometryList = null;
        clusteredGeometryList = new ArrayList<>();
        clusteredRootGeometry = null;
        visibilityQueryRecorder = null;
        faceTable = null;
        numberOfFaces = 0;
        faceBounds = null;
        accelerationMethod = SceneAccelerationMethod.WALD2007_ACCELERATION;
        sceneQuery = null;
        sceneQueryWorkspace = null;
        patchList = null;
        lightSourcePatchList = null;
    }

    public void destroy() {
        if (lightSourcePatchList != null) {
            lightSourcePatchList = null;
        }
        if (clusteredGeometryList != null) {
            clusteredGeometryList = null;
        }
        if (clusteredRootGeometry != null) {
            // This is deleted on Cluster::deleteCachedGeometries()
            clusteredRootGeometry = null;
        }
        if (patchList != null) {
            patchList = null;
        }
        sceneQueryWorkspace = null;
        if (sceneQuery != null) {
            sceneQuery.destroy();
            sceneQuery = null;
        }
        if (visibilityQueryRecorder != null) {
            visibilityQueryRecorder.close();
            visibilityQueryRecorder = null;
        }
        faceBounds = null;
        faceTable = null;
        numberOfFaces = 0;
        if (background != null) {
            background = null;
        }
        if (camera != null) {
            camera = null;
        }
    }

    private static String printGeometryType(int id) {
        String response = unknownType;
        if (id == GeometryClassId.SURFACE_MESH) {
            response = meshSurfaceType;
        }
        else if (id == GeometryClassId.COMPOUND) {
            response = compoundType;
        }
        else if (id == GeometryClassId.PATCH_SET) {
            response = patchSetType;
        }
        return response;
    }

    private static void printPatchSet(PatchSet patchSet) {
        if (patchSet.getPatchList() != null) {
            System.out.printf("  - Patches: %d\n", patchSet.getPatchList().size());
        }
        else {
            System.out.printf("  - Patches: null list!\n");
        }
    }

    private static void printCompound(Compound compound) {
        if (compound.children != null) {
            System.out.printf("    . Outer children: %d\n", compound.children.size());
            for (int i = 0; i < compound.children.size(); i++) {
                Geometry child = compound.children.get(i);
                System.out.printf("    . Child [%d] / [%s]\n", i, printGeometryType(child.className));
                if (child.className == GeometryClassId.SURFACE_MESH) {
                    printSurfaceMesh((MeshSurface)child, 6);
                }
            }
        }
    }

    private static void printSurfaceMesh(MeshSurface mesh, int level) {
        StringBuilder spacesBuilder = new StringBuilder();

        for (int i = 0; i < level; i++) {
            spacesBuilder.append(' ');
        }

        String spaces = spacesBuilder.toString();

        System.out.printf("%s  - Object name: %s\n", spaces, mesh.objectName);
        System.out.printf("%s  - Type: SurfaceMesh\n", spaces);
        System.out.printf("%s    . Id: %d\n", spaces, mesh.id);
        System.out.printf("%s    . Inner id: %d\n", spaces, mesh.meshId);
        System.out.printf("%s    . Vertices: %d, positions: %d, normals: %d, faces: %d\n",
            spaces, mesh.vertices.size(), mesh.positions.size(), mesh.normals.size(), mesh.faces.size());
    }

    private void printGeometries() {
        System.out.printf("= geometryList ================================================================\n");
        System.out.printf("Geometries on list: %d\n", geometryList.size());
        for (int i = 0; i < geometryList.size(); i++) {
            Geometry geometry = geometryList.get(i);
            System.out.printf("  - Index: [%d of %d] / [%s]\n", i + 1, geometryList.size(), printGeometryType(geometry.className));
            System.out.printf("    . Id: %d\n", geometry.id);
            System.out.printf("    . %s\n", geometry.isDuplicate ? "Duplicate" : "Original");

            if (geometry.className == GeometryClassId.SURFACE_MESH) {
                // Note that empty meshes are being removed, this case will usually not show on Galerkin
                printSurfaceMesh((MeshSurface)geometry, 0);
            }
            else if (geometry.className == GeometryClassId.COMPOUND) {
                printCompound((Compound)geometry);
            }
            else if (geometry.className == GeometryClassId.PATCH_SET) {
                printPatchSet((PatchSet)geometry);
            }
        }
    }

    private void printClusteredGeometries() {
        System.out.printf("= clusteredGeometryList ================================================================\n");
        System.out.printf("Geometry clusters on list: %d\n", clusteredGeometryList.size());
        for (int i = 0; i < clusteredGeometryList.size(); i++) {
            Geometry geometry = clusteredGeometryList.get(i);
            System.out.printf("  - Index: [%d of %d] / [%s]\n", i + 1, clusteredGeometryList.size(), printGeometryType(geometry.className));
            System.out.printf("    . Id: %d\n", geometry.id);
            System.out.printf("    . %s\n", geometry.isDuplicate ? "Duplicate" : "Original");

            if (geometry.className == GeometryClassId.SURFACE_MESH) {
                // Note that empty meshes are being removed, this case will usually not show on Galerkin
                printSurfaceMesh((MeshSurface)geometry, 0);
            }
            else if (geometry.className == GeometryClassId.COMPOUND) {
                printCompound((Compound)geometry);
            }
            else if (geometry.className == GeometryClassId.PATCH_SET) {
                printPatchSet((PatchSet)geometry);
            }
        }
    }

    private void printPatches() {
        System.out.printf("= patchList ================================================================\n");
        if (patchList == null) {
            System.out.printf("Patches on top level scene list: NULL\n");
            return;
        }
        System.out.printf("Patches on top level scene list: %d\n", patchList.size());
        for (int i = 0; i < patchList.size(); i++) {
            Patch patch = patchList.get(i);
            System.out.printf("  - patch[%d]: vertices: %d, area: %03f\n",
                i, patch.numberOfVertices, patch.area);
        }
    }

    private static void printClusterHierarchy(Geometry node, int level, int[] elementCount) {
        if (level == 0) {
            System.out.printf("= clusteredRootGeometry ================================================================\n");
        }
        switch (level) {
            case 0:
                break;
            case 1:
                System.out.printf("* ");
                break;
            case 2:
                System.out.printf("  - ");
                break;
            case 3:
                System.out.printf("    . ");
                break;
            default:
                System.out.printf("   ");
                for (int j = 0; j < level; j++) {
                    System.out.printf(" ");
                }
                System.out.printf("[%d] ", level);
                break;
        }
        if (node.className == GeometryClassId.SURFACE_MESH) {
            // Note that empty meshes are being removed, this case will usually not show on Galerkin
            System.out.printf("Mesh (%d)\n", elementCount[0]);
            elementCount[0]++;
        }
        else if (node.className == GeometryClassId.COMPOUND) {
            Compound compound = (Compound)node;
            System.out.printf("Compound %d (%d)\n", compound.id, elementCount[0]);
            elementCount[0]++;
            for (int i = 0;
                 compound.children != null && i < compound.children.size();
                 i++) {
                printClusterHierarchy(compound.children.get(i), level + 1, elementCount);
            }

        }
        else if (node.className == GeometryClassId.PATCH_SET) {
            PatchSet patchSet = (PatchSet)node;
            if (patchSet.getPatchList() == null) {
                System.out.printf("empty PatchSet (%d)\n", elementCount[0]);
            }
            else {
                System.out.printf("PatchSet %d with %d patches (%d)\n", patchSet.id, patchSet.getPatchList().size(), elementCount[0]);
            }
            elementCount[0]++;
        }
    }

    private void printSceneQuery() {
        System.out.printf("= scene query (%s) ===========================================================\n",
            SceneQuery.methodName(accelerationMethod));
        if (sceneQuery != null && sceneQuery.getLegacyGrid() != null) {
            sceneQuery.getLegacyGrid().print();
        }
    }

    /** The index buildSceneQuery uses; set while the scene is being built */
    public void setAccelerationMethod(SceneAccelerationMethod method) {
        accelerationMethod = method;
    }

    public SceneAccelerationMethod getAccelerationMethod() {
        return accelerationMethod;
    }

    /**
    Builds the scene query over the face table with the acceleration method,
    and the scene's workspace for it (reporting to visibilityQueryRecorder in
    ray capture runs). Called once, after buildFaceTable.
    */
    public void buildSceneQuery() {
        if (sceneQuery != null) {
            sceneQuery.destroy();
        }
        sceneQuery = new SceneQuery(this, accelerationMethod);
        sceneQueryWorkspace = new SceneQueryWorkspace(sceneQuery, visibilityQueryRecorder);
    }

    public SceneQuery getSceneQuery() {
        return sceneQuery;
    }

    public SceneQueryWorkspace getSceneQueryWorkspace() {
        return sceneQueryWorkspace;
    }

    private static void collectFaces(Geometry geometry, Patch[] table, int capacity, int[] count, int[] conflicts) {
        if (geometry == null) {
            return;
        }
        // Faces are numbered depth-first, so the faces below this geometry are
        // exactly the interval [first, count[0] - 1] once its subtree is visited
        final int first = count[0];
        if (geometry.className == GeometryClassId.COMPOUND) {
            ArrayList<Geometry> children = ((Compound)geometry).children;
            if (children != null) {
                for (int i = 0; i < children.size(); i++) {
                    collectFaces(children.get(i), table, capacity, count, conflicts);
                }
            }
            geometry.setFaceIndexRange(first, count[0] - 1);
            return;
        }
        ArrayList<Patch> patches = Geometry.patchListReference(geometry);
        for (int i = 0; patches != null && i < patches.size(); i++) {
            Patch patch = patches.get(i);
            if (patch.getFaceIndex() >= 0 || count[0] >= capacity) {
                conflicts[0]++;
                continue;
            }
            patch.setFaceIndex(count[0]);
            table[count[0]] = patch;
            count[0]++;
        }
        geometry.setFaceIndexRange(first, count[0] - 1);
    }

    /**
    Assigns the dense face indices and builds the face table and the face
    bounds. Called once, after the cluster hierarchy exists and before the
    scene query. A patch reachable from two geometries, or a patch of
    patchList that clusteredRootGeometry does not reach, is a fatal error.
    */
    public void buildFaceTable() {
        faceBounds = null;
        faceTable = null;
        numberOfFaces = 0;
        final int capacity = patchList != null ? patchList.size() : 0;
        for (int i = 0; i < capacity; i++) {
            patchList.get(i).setFaceIndex(-1);
        }
        faceTable = new Patch[capacity > 0 ? capacity : 1];
        int[] count = new int[] {0};
        int[] conflicts = new int[] {0};
        collectFaces(clusteredRootGeometry, faceTable, capacity, count, conflicts);
        numberOfFaces = count[0];

        int unreached = 0;
        for (int i = 0; i < capacity; i++) {
            if (patchList.get(i).getFaceIndex() < 0) {
                unreached++;
            }
        }
        if (conflicts[0] != 0 || unreached != 0 || numberOfFaces != capacity) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "buildFaceTable", String.format(
                "Face table: %d patches, %d indexed, %d reached twice, %d unreachable from the cluster hierarchy",
                capacity, numberOfFaces, conflicts[0], unreached));
        }
        faceBounds = new FaceBoundsTable(faceTable, numberOfFaces);
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public Patch getFace(int faceIndex) {
        return faceTable[faceIndex];
    }

    public FaceBoundsTable getFaceBounds() {
        return faceBounds;
    }

    public void print() {
        printGeometries();
        printClusteredGeometries();
        printPatches();
        int[] elementCount = new int[] {0};
        printClusterHierarchy(clusteredRootGeometry, 0, elementCount);
        printSceneQuery();
        System.out.printf("*** Total number of geometry elements on cluster hierarchy: %d\n", elementCount[0]);
    }
}
