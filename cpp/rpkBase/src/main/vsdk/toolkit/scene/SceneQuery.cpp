#include <cstring>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/LinearSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneFaceRayTest.h"
#include "vsdk/toolkit/scene/SceneQuery.h"

static void
boxValues(const RpkAxisAlignedBoundingBox &box, double *values) {
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
static void
addMembers(SpatialGroupTree *tree, const int group, const RpkGeometry *geometry) {
    if ( geometry->isCompound() ) {
        const java::ArrayList<RpkGeometry *> *children = static_cast<const Compound *>(geometry)->children;
        for ( int i = 0; children != nullptr && i < children->size(); i++ ) {
            const RpkGeometry *child = children->get(i);
            double box[6];
            boxValues(child->boundingBox, box);
            addMembers(tree, tree->addGroup(group, child->bounded ? box : nullptr), child);
        }
        return;
    }
    const java::ArrayList<Patch *> *patches = RpkGeometry::patchListReference(geometry);
    for ( int i = 0; patches != nullptr && i < patches->size(); i++ ) {
        tree->addPrimitive(group, patches->get(i)->getFaceIndex());
    }
}

SceneQuery::SceneQuery(const Scene *inScene, const SceneAccelerationMethod inMethod):
    scene(inScene),
    method(inMethod),
    index(buildIndex(*inScene, inMethod))
{
}

SceneQuery::~SceneQuery() {
    delete index;
    index = nullptr;
}

SpatialIndex *
SceneQuery::buildIndex(const Scene &scene, const SceneAccelerationMethod method) {
    const FaceBoundsTable *faceBounds = scene.getFaceBounds();
    const int numberOfFaces = faceBounds->getNumberOfFaces();

    if ( method == SceneAccelerationMethod::BRUTE_FORCE_ACCELERATION ) {
        return new LinearSpatialIndex(numberOfFaces);
    }

    if ( method == SceneAccelerationMethod::WALD2007_ACCELERATION ) {
        java::ArrayList<double> boxes(6L * numberOfFaces);
        for ( int i = 0; i < numberOfFaces; i++ ) {
            const RpkAxisAlignedBoundingBox &box = faceBounds->get(i);
            const double extent = box.maxExtent();
            const double low[3] = {box.minX(), box.minY(), box.minZ()};
            const double high[3] = {box.maxX(), box.maxY(), box.maxZ()};
            const double tolerance = scene.getFace(i)->getTolerance();
            for ( int axis = 0; axis < 3; axis++ ) {
                const double magnitude = java::Math::max(java::Math::abs(low[axis]), java::Math::abs(high[axis]));
                const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                boxes.add(low[axis] - padding);
            }
            for ( int axis = 0; axis < 3; axis++ ) {
                const double magnitude = java::Math::max(java::Math::abs(low[axis]), java::Math::abs(high[axis]));
                const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                boxes.add(high[axis] + padding);
            }
        }
        return new BinnedSahSpatialIndex(boxes, java::ArrayList<int>());
    }

    if ( method == SceneAccelerationMethod::VOXEL_ACCELERATION ) {
        java::ArrayList<double> rawBoxes(6L * numberOfFaces);
        for ( int i = 0; i < numberOfFaces; i++ ) {
            double box[6];
            boxValues(faceBounds->get(i), box);
            for ( int k = 0; k < 6; k++ ) {
                rawBoxes.add(box[k]);
            }
        }
        const RpkGeometry *root = scene.clusteredRootGeometry;
        double rootBox[6];
        boxValues(root->boundingBox, rootBox);
        SpatialGroupTree tree(rawBoxes, rootBox);
        addMembers(&tree, 0, root);
        return new GridSpatialIndex(tree);
    }

    return nullptr;
}

const char *
SceneQuery::methodName(const SceneAccelerationMethod method) {
    switch ( method ) {
        case SceneAccelerationMethod::VOXEL_ACCELERATION:
            return "voxel";
        case SceneAccelerationMethod::WALD2007_ACCELERATION:
            return "wald2007";
        case SceneAccelerationMethod::BRUTE_FORCE_ACCELERATION:
            return "bruteforce";
    }
    return "unknown";
}

bool
SceneQuery::methodFromName(const char *name, SceneAccelerationMethod *method) {
    static const SceneAccelerationMethod METHODS[] = {
        SceneAccelerationMethod::VOXEL_ACCELERATION,
        SceneAccelerationMethod::WALD2007_ACCELERATION,
        SceneAccelerationMethod::BRUTE_FORCE_ACCELERATION
    };
    if ( name == nullptr ) {
        return false;
    }
    for ( int i = 0; i < 3; i++ ) {
        if ( strcmp(name, methodName(METHODS[i])) == 0 ) {
            *method = METHODS[i];
            return true;
        }
    }
    return false;
}

RpkRayHit *
SceneQuery::intersect(
    Ray *ray,
    const double minimumDistance,
    double *maximumDistance,
    const int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions,
    SceneQueryWorkspace *workspace) const
{
    SceneFaceRayTest test(*scene, ray, hitFlags, exclusions, hitStore);
    const Ray indexRay = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
    const SpatialQueryMode mode = (hitFlags & RayHitFlag::ANY) != 0
        ? SpatialQueryMode::ANY_HIT : SpatialQueryMode::CLOSEST_HIT;
    // The ported grid takes the face test by its type, so the compiler can
    // inline it into the cell loop (no virtual call per candidate face)
    const SpatialQueryResult result = method == SceneAccelerationMethod::VOXEL_ACCELERATION
        ? static_cast<const GridSpatialIndex *>(index)->queryWith(
            indexRay, minimumDistance, *maximumDistance, mode, test, *workspace->getIndexWorkspace())
        : index->query(indexRay, minimumDistance, *maximumDistance, mode, test, *workspace->getIndexWorkspace());
    if ( !result.hasHit() ) {
        return nullptr;
    }
    *maximumDistance = static_cast<double>(result.getParameter());
    return hitStore;
}

SceneQueryWorkspace::SceneQueryWorkspace(const SceneQuery &query):
    indexWorkspace(new SpatialIndexWorkspace(query.getIndex()->getWorkspaceSize()))
{
}

SceneQueryWorkspace::~SceneQueryWorkspace() {
    delete indexWorkspace;
    indexWorkspace = nullptr;
}
