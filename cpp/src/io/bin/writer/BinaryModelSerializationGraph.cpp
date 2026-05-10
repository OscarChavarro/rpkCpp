#include <cstddef>

#include "java/util/ArrayList.txx"
#include "java/util/HashMap.txx"
#include "common/logging/Logger.h"
#include "material/Material.h"
#include "skin/Compound.h"
#include "skin/Geometry.h"
#include "skin/MeshSurface.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/PatchSet.h"
#include "environment/geometry/elements/Vertex.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/writer/BinaryModelSerializationGraph.h"

bool
BinaryModelSerializationGraph::ensureVector(const Vector3D *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( vectorIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(vectors.size());
    if ( !vectors.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureVector", "Failed to append vector");
        return false;
    }
    if ( !vectorIndices.put(value, index) ) {
        vectors.remove(vectors.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureVector", "Failed to index vector");
        return false;
    }
    return true;
}

bool
BinaryModelSerializationGraph::ensureMaterial(const Material *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( materialIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(materials.size());
    if ( !materials.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureMaterial", "Failed to append material");
        return false;
    }
    if ( !materialIndices.put(value, index) ) {
        materials.remove(materials.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureMaterial", "Failed to index material");
        return false;
    }
    return true;
}

bool
BinaryModelSerializationGraph::ensureVertex(const Vertex *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( vertexIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->radianceData != nullptr ) {
        Logger::error("BinaryModelSerializationGraph::ensureVertex", "Vertex radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(vertices.size());
    if ( !vertices.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureVertex", "Failed to append vertex");
        return false;
    }
    if ( !vertexIndices.put(value, index) ) {
        vertices.remove(vertices.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureVertex", "Failed to index vertex");
        return false;
    }

    if ( !ensureVector(value->point) ) {
        return false;
    }
    if ( !ensureVector(value->normal) ) {
        return false;
    }
    if ( !ensureVector(value->textureCoordinates) ) {
        return false;
    }
    if ( !ensureVertex(value->back) ) {
        return false;
    }

    if ( value->patches != nullptr ) {
        for ( int i = 0; i < value->patches->size(); i++ ) {
            if ( !ensurePatch(value->patches->get(i)) ) {
                return false;
            }
        }
    }

    return true;
}

bool
BinaryModelSerializationGraph::ensurePatch(const Patch *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( patchIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->getRadianceData() != nullptr ) {
        Logger::error("BinaryModelSerializationGraph::ensurePatch", "Patch radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(patches.size());
    if ( !patches.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensurePatch", "Failed to append patch");
        return false;
    }
    if ( !patchIndices.put(value, index) ) {
        patches.remove(patches.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensurePatch", "Failed to index patch");
        return false;
    }

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        if ( !ensureVertex(value->getVertices()[i]) ) {
            return false;
        }
    }
    if ( !ensurePatch(value->getTwin()) ) {
        return false;
    }
    if ( !ensureMaterial(value->getMaterial()) ) {
        return false;
    }

    return true;
}

bool
BinaryModelSerializationGraph::ensureGeometry(const Geometry *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( geometryIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->radianceData != nullptr ) {
        Logger::error("BinaryModelSerializationGraph::ensureGeometry", "Geometry radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(geometries.size());
    if ( !geometries.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureGeometry", "Failed to append geometry");
        return false;
    }
    if ( !geometryIndices.put(value, index) ) {
        geometries.remove(geometries.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureGeometry", "Failed to index geometry");
        return false;
    }

    if ( value->className == GeometryClassId::SURFACE_MESH ) {
        const MeshSurface *surface = static_cast<const MeshSurface *>(value);
        if ( !ensureMaterial(surface->material) ) {
            return false;
        }
        if ( !collectVectorList(surface->positions) ) {
            return false;
        }
        if ( !collectVectorList(surface->normals) ) {
            return false;
        }
        if ( !collectVertexList(surface->vertices) ) {
            return false;
        }
        if ( !collectPatchList(surface->faces) ) {
            return false;
        }
    } else if ( value->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(value);
        if ( !collectGeometryList(compound->children) ) {
            return false;
        }
    } else if ( value->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(value);
        if ( !collectPatchList(patchSet->getPatchList()) ) {
            return false;
        }
    } else {
        Logger::error("BinaryModelSerializationGraph::ensureGeometry", "Unsupported geometry class for BinaryModelWritter");
        return false;
    }

    return true;
}

bool
BinaryModelSerializationGraph::ensureColorContext(const ColorContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( colorContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(colorContexts.size());
    if ( !colorContexts.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureColorContext", "Failed to append color context");
        return false;
    }
    if ( !colorContextIndices.put(value, index) ) {
        colorContexts.remove(colorContexts.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureColorContext", "Failed to index color context");
        return false;
    }
    return true;
}

bool
BinaryModelSerializationGraph::ensureReaderContext(const ReaderContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( readerContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(readerContexts.size());
    if ( !readerContexts.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureReaderContext", "Failed to append reader context");
        return false;
    }
    if ( !readerContextIndices.put(value, index) ) {
        readerContexts.remove(readerContexts.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureReaderContext", "Failed to index reader context");
        return false;
    }

    return ensureReaderContext(value->prev);
}

bool
BinaryModelSerializationGraph::ensureTransformArray(const TransformSequenceContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( transformArrayIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(transformArrays.size());
    if ( !transformArrays.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureTransformArray", "Failed to append transform array");
        return false;
    }
    if ( !transformArrayIndices.put(value, index) ) {
        transformArrays.remove(transformArrays.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureTransformArray", "Failed to index transform array");
        return false;
    }
    return true;
}

bool
BinaryModelSerializationGraph::ensureTransformContext(const TransformStackContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( transformContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(transformContexts.size());
    if ( !transformContexts.add(value) ) {
        Logger::error("BinaryModelSerializationGraph::ensureTransformContext", "Failed to append transform context");
        return false;
    }
    if ( !transformContextIndices.put(value, index) ) {
        transformContexts.remove(transformContexts.size() - 1);
        Logger::error("BinaryModelSerializationGraph::ensureTransformContext", "Failed to index transform context");
        return false;
    }

    if ( !ensureTransformArray(value->transformationArray) ) {
        return false;
    }
    return ensureTransformContext(value->prev);
}

bool
BinaryModelSerializationGraph::collectVectorList(const java::ArrayList<Vector3D *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureVector(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelSerializationGraph::collectVertexList(const java::ArrayList<Vertex *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureVertex(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelSerializationGraph::collectPatchList(const java::ArrayList<Patch *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensurePatch(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelSerializationGraph::collectMaterialList(const java::ArrayList<Material *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureMaterial(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelSerializationGraph::collectGeometryList(const java::ArrayList<Geometry *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureGeometry(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelSerializationGraph::collectModel(const ParseSnapshotContext *model) {
    if ( model == nullptr ) {
        return true;
    }

    if ( !ensureColorContext(model->currentColor) ) {
        return false;
    }
    if ( !collectPatchList(model->currentFaceList) ) {
        return false;
    }
    if ( !collectGeometryList(model->currentGeometryList) ) {
        return false;
    }
    if ( !collectMaterialList(model->materials) ) {
        return false;
    }
    if ( !collectVectorList(model->currentNormalList) ) {
        return false;
    }
    if ( !collectVectorList(model->currentPointList) ) {
        return false;
    }
    if ( !collectVertexList(model->currentVertexList) ) {
        return false;
    }
    if ( !collectGeometryList(model->geometries) ) {
        return false;
    }
    if ( !ensureReaderContext(model->readerContext) ) {
        return false;
    }
    return ensureTransformContext(model->transformContext);
}
