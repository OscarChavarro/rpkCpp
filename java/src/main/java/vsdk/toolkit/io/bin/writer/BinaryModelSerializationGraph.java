package vsdk.toolkit.io.bin.writer;

import java.util.ArrayList;
import java.util.IdentityHashMap;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.PatchSet;
import vsdk.toolkit.skin.Vertex;

public class BinaryModelSerializationGraph {
    public IdentityHashMap<Vector3D, Integer> vectorIndices = new IdentityHashMap<>();
    public ArrayList<Vector3D> vectors = new ArrayList<>();

    public IdentityHashMap<Vertex, Integer> vertexIndices = new IdentityHashMap<>();
    public ArrayList<Vertex> vertices = new ArrayList<>();

    public IdentityHashMap<Patch, Integer> patchIndices = new IdentityHashMap<>();
    public ArrayList<Patch> patches = new ArrayList<>();

    public IdentityHashMap<Material, Integer> materialIndices = new IdentityHashMap<>();
    public ArrayList<Material> materials = new ArrayList<>();

    public IdentityHashMap<Geometry, Integer> geometryIndices = new IdentityHashMap<>();
    public ArrayList<Geometry> geometries = new ArrayList<>();

    public IdentityHashMap<ColorContext, Integer> colorContextIndices = new IdentityHashMap<>();
    public ArrayList<ColorContext> colorContexts = new ArrayList<>();

    public IdentityHashMap<ReaderContext, Integer> readerContextIndices = new IdentityHashMap<>();
    public ArrayList<ReaderContext> readerContexts = new ArrayList<>();

    public IdentityHashMap<TransformSequenceContext, Integer> transformArrayIndices = new IdentityHashMap<>();
    public ArrayList<TransformSequenceContext> transformArrays = new ArrayList<>();

    public IdentityHashMap<TransformStackContext, Integer> transformContextIndices = new IdentityHashMap<>();
    public ArrayList<TransformStackContext> transformContexts = new ArrayList<>();

    public boolean ensureVector(Vector3D value) {
        if (value == null) {
            return true;
        }
        if (vectorIndices.containsKey(value)) {
            return true;
        }
        int index = vectors.size();
        vectors.add(value);
        vectorIndices.put(value, index);
        return true;
    }

    public boolean ensureMaterial(Material value) {
        if (value == null) {
            return true;
        }
        if (materialIndices.containsKey(value)) {
            return true;
        }
        int index = materials.size();
        materials.add(value);
        materialIndices.put(value, index);
        return true;
    }

    public boolean ensureVertex(Vertex value) {
        if (value == null) {
            return true;
        }
        if (vertexIndices.containsKey(value)) {
            return true;
        }

        if (value.radianceData != null) {
            Logger.error("BinaryModelSerializationGraph::ensureVertex", "Vertex radianceData is not supported by BinaryModelWritter");
            return false;
        }

        int index = vertices.size();
        vertices.add(value);
        vertexIndices.put(value, index);

        if (!ensureVector(value.point)) {
            return false;
        }
        if (!ensureVector(value.normal)) {
            return false;
        }
        if (!ensureVector(value.textureCoordinates)) {
            return false;
        }
        if (!ensureVertex(value.back)) {
            return false;
        }

        if (value.patches != null) {
            for (int i = 0; i < value.patches.size(); i++) {
                if (!ensurePatch(value.patches.get(i))) {
                    return false;
                }
            }
        }

        return true;
    }

    public boolean ensurePatch(Patch value) {
        if (value == null) {
            return true;
        }
        if (patchIndices.containsKey(value)) {
            return true;
        }

        if (value.radianceData != null) {
            Logger.error("BinaryModelSerializationGraph::ensurePatch", "Patch radianceData is not supported by BinaryModelWritter");
            return false;
        }

        int index = patches.size();
        patches.add(value);
        patchIndices.put(value, index);

        for (int i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
            if (!ensureVertex(value.vertex[i])) {
                return false;
            }
        }
        if (!ensurePatch(value.twin)) {
            return false;
        }
        if (!ensureMaterial(value.material)) {
            return false;
        }

        return true;
    }

    public boolean ensureGeometry(Geometry value) {
        if (value == null) {
            return true;
        }
        if (geometryIndices.containsKey(value)) {
            return true;
        }

        if (value.radianceData != null) {
            Logger.error("BinaryModelSerializationGraph::ensureGeometry", "Geometry radianceData is not supported by BinaryModelWritter");
            return false;
        }

        int index = geometries.size();
        geometries.add(value);
        geometryIndices.put(value, index);

        if (value.className == GeometryClassId.SURFACE_MESH) {
            MeshSurface surface = (MeshSurface)value;
            if (!ensureMaterial(surface.material)) {
                return false;
            }
            if (!collectVectorList(surface.positions)) {
                return false;
            }
            if (!collectVectorList(surface.normals)) {
                return false;
            }
            if (!collectVertexList(surface.vertices)) {
                return false;
            }
            if (!collectPatchList(surface.faces)) {
                return false;
            }
        }
        else if (value.className == GeometryClassId.COMPOUND) {
            Compound compound = (Compound)value;
            if (!collectGeometryList(compound.children)) {
                return false;
            }
        }
        else if (value.className == GeometryClassId.PATCH_SET) {
            PatchSet patchSet = (PatchSet)value;
            if (!collectPatchList(patchSet.getPatchList())) {
                return false;
            }
        }
        else {
            Logger.error("BinaryModelSerializationGraph::ensureGeometry", "Unsupported geometry class for BinaryModelWritter");
            return false;
        }

        return true;
    }

    public boolean ensureColorContext(ColorContext value) {
        if (value == null) {
            return true;
        }
        if (colorContextIndices.containsKey(value)) {
            return true;
        }
        int index = colorContexts.size();
        colorContexts.add(value);
        colorContextIndices.put(value, index);
        return true;
    }

    public boolean ensureReaderContext(ReaderContext value) {
        if (value == null) {
            return true;
        }
        if (readerContextIndices.containsKey(value)) {
            return true;
        }
        int index = readerContexts.size();
        readerContexts.add(value);
        readerContextIndices.put(value, index);
        return ensureReaderContext(value.prev);
    }

    public boolean ensureTransformArray(TransformSequenceContext value) {
        if (value == null) {
            return true;
        }
        if (transformArrayIndices.containsKey(value)) {
            return true;
        }
        int index = transformArrays.size();
        transformArrays.add(value);
        transformArrayIndices.put(value, index);
        return true;
    }

    public boolean ensureTransformContext(TransformStackContext value) {
        if (value == null) {
            return true;
        }
        if (transformContextIndices.containsKey(value)) {
            return true;
        }
        int index = transformContexts.size();
        transformContexts.add(value);
        transformContextIndices.put(value, index);

        if (!ensureTransformArray(value.transformationArray)) {
            return false;
        }
        return ensureTransformContext(value.prev);
    }

    public boolean collectVectorList(ArrayList<Vector3D> list) {
        if (list == null) {
            return true;
        }
        for (int i = 0; i < list.size(); i++) {
            if (!ensureVector(list.get(i))) {
                return false;
            }
        }
        return true;
    }

    public boolean collectVertexList(ArrayList<Vertex> list) {
        if (list == null) {
            return true;
        }
        for (int i = 0; i < list.size(); i++) {
            if (!ensureVertex(list.get(i))) {
                return false;
            }
        }
        return true;
    }

    public boolean collectPatchList(ArrayList<Patch> list) {
        if (list == null) {
            return true;
        }
        for (int i = 0; i < list.size(); i++) {
            if (!ensurePatch(list.get(i))) {
                return false;
            }
        }
        return true;
    }

    public boolean collectMaterialList(ArrayList<Material> list) {
        if (list == null) {
            return true;
        }
        for (int i = 0; i < list.size(); i++) {
            if (!ensureMaterial(list.get(i))) {
                return false;
            }
        }
        return true;
    }

    public boolean collectGeometryList(ArrayList<Geometry> list) {
        if (list == null) {
            return true;
        }
        for (int i = 0; i < list.size(); i++) {
            if (!ensureGeometry(list.get(i))) {
                return false;
            }
        }
        return true;
    }

    public boolean collectModel(ParseSnapshotContext model) {
        if (model == null) {
            return true;
        }

        if (!ensureColorContext(model.currentColor)) {
            return false;
        }
        if (!collectPatchList(model.currentFaceList)) {
            return false;
        }
        if (!collectGeometryList(model.currentGeometryList)) {
            return false;
        }
        if (!collectMaterialList(model.materials)) {
            return false;
        }
        if (!collectVectorList(model.currentNormalList)) {
            return false;
        }
        if (!collectVectorList(model.currentPointList)) {
            return false;
        }
        if (!collectVertexList(model.currentVertexList)) {
            return false;
        }
        if (!collectGeometryList(model.geometries)) {
            return false;
        }
        if (!ensureReaderContext(model.readerContext)) {
            return false;
        }
        return ensureTransformContext(model.transformContext);
    }
}
