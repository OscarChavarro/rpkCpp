#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "skin/Compound.h"
#include "galerkin/GalerkinGeometry.h"

void
GalerkinGeometry::validateGeometry(const Geometry *geometry, const char *context) {
    if ( geometry == nullptr ) {
        return;
    }

    if ( geometry->className == GeometryClassId::PATCH_SET ) {
        return;
    }

    if ( geometry->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(geometry);
        for ( int i = 0; compound->children != nullptr && i < compound->children->size(); i++ ) {
            validateGeometry(compound->children->get(i), context);
        }
        return;
    }

    Error::fatal(
        -1,
        context,
        "Galerkin only supports PatchSet/Compound geometry trees. Found class id %d",
        static_cast<int>(geometry->className));
}

void
GalerkinGeometry::validateSceneForGalerkin(const Scene *scene) {
    if ( scene == nullptr ) {
        Error::fatal(-1, "validateSceneForGalerkin", "Null scene");
    }

    // Galerkin hot paths consume the clustered hierarchy. The original scene
    // list may still contain SURFACE_MESH entries, so we validate the clustered
    // root (which is expected to be Compound/PatchSet only).
    validateGeometry(scene->clusteredRootGeometry, "validateSceneForGalerkin(scene->clusteredRootGeometry)");
}

void
GalerkinGeometry::collectPatchSetsRecursive(
    const Geometry *geometry,
    java::ArrayList<PatchSet *> *patchSets)
{
    if ( geometry == nullptr || patchSets == nullptr ) {
        return;
    }

    if ( geometry->className == GeometryClassId::PATCH_SET ) {
        patchSets->add(static_cast<PatchSet *>(const_cast<Geometry *>(geometry)));
        return;
    }

    if ( geometry->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(geometry);
        for ( int i = 0; compound->children != nullptr && i < compound->children->size(); i++ ) {
            collectPatchSetsRecursive(compound->children->get(i), patchSets);
        }
        return;
    }

    Error::fatal(
        -1,
        "collectPatchSetsRecursive",
        "Unsupported geometry class id %d while collecting PatchSet lists",
        static_cast<int>(geometry->className));
}

java::ArrayList<PatchSet *> *
GalerkinGeometry::collectPatchSets(const java::ArrayList<Geometry *> *geometryList) {
    java::ArrayList<PatchSet *> *patchSets = new java::ArrayList<PatchSet *>();
    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        collectPatchSetsRecursive(geometryList->get(i), patchSets);
    }
    return patchSets;
}

java::ArrayList<PatchSet *> *
GalerkinGeometry::collectPatchSets(const Geometry *geometry) {
    java::ArrayList<PatchSet *> *patchSets = new java::ArrayList<PatchSet *>();
    collectPatchSetsRecursive(geometry, patchSets);
    return patchSets;
}
