#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "FaceAncestry.h"

FaceAncestry::FaceAncestry(const Scene &scene):
    geometryById(),
    parentById(),
    leafByFace()
{
    for ( int i = 0; i < scene.getFaceBounds()->getNumberOfFaces(); i++ ) {
        leafByFace.add(-1);
    }
    visit(scene.clusteredRootGeometry, -1, scene);
}

void
FaceAncestry::visit(const Geometry *geometry, const int parentId, const Scene &scene) {
    const int id = geometry->id;
    while ( geometryById.size() <= id ) {
        geometryById.add(NULL);
        parentById.add(-1);
    }
    geometryById.set(id, geometry);
    parentById.set(id, parentId);
    if ( geometry->isCompound() ) {
        const ArrayList<Geometry *> *children = ((const Compound *)(geometry))->children;
        for ( long i = 0; children != NULL && i < children->size(); i++ ) {
            visit(children->get(i), id, scene);
        }
        return;
    }
    const ArrayList<Patch *> *patches = Geometry::patchListReference(geometry);
    for ( long i = 0; patches != NULL && i < patches->size(); i++ ) {
        const int face = patches->get(i)->getFaceIndex();
        if ( face >= 0 && leafByFace.get(face) < 0 ) {
            leafByFace.set(face, id);
        }
    }
    (void)scene;
}

const Geometry *
FaceAncestry::getGeometry(const long id) const {
    return id >= 0 && id < geometryById.size() ? geometryById.get(id) : NULL;
}

bool
FaceAncestry::excludesFace(const int faceIndex, const RayExclusionSet &exclusions) const {
    if ( exclusions.getExcludedGeometry1() == NULL && exclusions.getExcludedGeometry2() == NULL ) {
        return false;
    }
    for ( int id = leafByFace.get(faceIndex); id >= 0; id = parentById.get(id) ) {
        if ( exclusions.excludesGeometry(geometryById.get(id)) ) {
            return true;
        }
    }
    return false;
}
