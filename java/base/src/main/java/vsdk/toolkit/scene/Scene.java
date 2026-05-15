package vsdk.toolkit.scene;

import java.util.ArrayList;
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

    public Background background;
    public Camera camera;
    public ArrayList<Geometry> geometryList;
    public ArrayList<Geometry> clusteredGeometryList;
    public Geometry clusteredRootGeometry;
    public VoxelGrid voxelGrid;

    // The list of all patches in the current scene
    public ArrayList<Patch> patchList;

    // The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
    public ArrayList<Patch> lightSourcePatchList;

    public Scene() {
        background = null;
        camera = new Camera();
        geometryList = null;
        clusteredGeometryList = new ArrayList<>();
        clusteredRootGeometry = null;
        voxelGrid = null;
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
        if (voxelGrid != null) {
            voxelGrid = null;
        }
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

    private void printVoxelGrid() {
        System.out.printf("= voxelGrid ================================================================\n");
        voxelGrid.print();
    }

    public void print() {
        printGeometries();
        printClusteredGeometries();
        printPatches();
        int[] elementCount = new int[] {0};
        printClusterHierarchy(clusteredRootGeometry, 0, elementCount);
        printVoxelGrid();
        System.out.printf("*** Total number of geometry elements on cluster hierarchy: %d\n", elementCount[0]);
    }
}
