#include <string.h>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/LinearSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneFaceRayTest.h"
#include "vsdk/toolkit/scene/SceneQuery.h"

static void
boxValues(const BoundingBox &box, double *values) {
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
static void
addMembers(SpatialGroupTree *tree, const int group, const Geometry *geometry) {
    if ( geometry->isCompound() ) {
        const ArrayList<Geometry *> *children = ((const Compound *)(geometry))->children;
        for ( int i = 0; children != NULL && i < children->size(); i++ ) {
            const Geometry *child = children->get(i);
            double box[6];
            boxValues(child->boundingBox, box);
            addMembers(tree, tree->addGroup(group, child->bounded ? box : NULL), child);
        }
        return;
    }
    const ArrayList<Patch *> *patches = Geometry::patchListReference(geometry);
    for ( int i = 0; patches != NULL && i < patches->size(); i++ ) {
        tree->addPrimitive(group, patches->get(i)->getFaceIndex());
    }
}

SceneQuery::SceneQuery(const Scene *inScene, const SceneAccelerationMethod inMethod):
    scene(inScene),
    method(inMethod),
    legacyGrid(NULL),
    index(NULL)
{
    if ( method == LEGACY_VOXEL_ACCELERATION ) {
        legacyGrid = new VoxelGrid(scene->clusteredRootGeometry);
    } else {
        index = buildIndex(*scene, method);
    }
}

SceneQuery::~SceneQuery() {
    delete legacyGrid;
    legacyGrid = NULL;
    delete index;
    index = NULL;
}

SpatialIndex *
SceneQuery::buildIndex(const Scene &scene, const SceneAccelerationMethod method) {
    const FaceBoundsTable *faceBounds = scene.getFaceBounds();
    const int numberOfFaces = faceBounds->getNumberOfFaces();

    if ( method == BRUTE_FORCE_ACCELERATION ) {
        return new LinearSpatialIndex(numberOfFaces);
    }

    if ( method == WALD2007_ACCELERATION ) {
        ArrayList<double> boxes(6L * numberOfFaces);
        for ( int i = 0; i < numberOfFaces; i++ ) {
            const BoundingBox &box = faceBounds->get(i);
            const double extent = box.maxExtent();
            double low[3];
            double high[3];
            low[0] = box.minX();
            low[1] = box.minY();
            low[2] = box.minZ();
            high[0] = box.maxX();
            high[1] = box.maxY();
            high[2] = box.maxZ();
            const double tolerance = scene.getFace(i)->getTolerance();
            int axis;
            for ( axis = 0; axis < 3; axis++ ) {
                const double magnitude = Math::max(Math::abs(low[axis]), Math::abs(high[axis]));
                const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                boxes.add(low[axis] - padding);
            }
            for ( axis = 0; axis < 3; axis++ ) {
                const double magnitude = Math::max(Math::abs(low[axis]), Math::abs(high[axis]));
                const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
                boxes.add(high[axis] + padding);
            }
        }
        return new BinnedSahSpatialIndex(boxes, ArrayList<int>());
    }

    if ( method == VOXEL_ACCELERATION ) {
        ArrayList<double> rawBoxes(6L * numberOfFaces);
        for ( int i = 0; i < numberOfFaces; i++ ) {
            double box[6];
            boxValues(faceBounds->get(i), box);
            for ( int k = 0; k < 6; k++ ) {
                rawBoxes.add(box[k]);
            }
        }
        const Geometry *root = scene.clusteredRootGeometry;
        double rootBox[6];
        boxValues(root->boundingBox, rootBox);
        SpatialGroupTree tree(rawBoxes, rootBox);
        addMembers(&tree, 0, root);
        return new GridSpatialIndex(tree);
    }

    return NULL;
}

const char *
SceneQuery::methodName(const SceneAccelerationMethod method) {
    switch ( method ) {
        case LEGACY_VOXEL_ACCELERATION:
            return "legacy";
        case VOXEL_ACCELERATION:
            return "voxel";
        case WALD2007_ACCELERATION:
            return "wald2007";
        case BRUTE_FORCE_ACCELERATION:
            return "bruteforce";
    }
    return "unknown";
}

bool
SceneQuery::methodFromName(const char *name, SceneAccelerationMethod *method) {
    static const SceneAccelerationMethod METHODS[] = {
        LEGACY_VOXEL_ACCELERATION,
        VOXEL_ACCELERATION,
        WALD2007_ACCELERATION,
        BRUTE_FORCE_ACCELERATION
    };
    if ( name == NULL ) {
        return false;
    }
    for ( int i = 0; i < 4; i++ ) {
        if ( strcmp(name, methodName(METHODS[i])) == 0 ) {
            *method = METHODS[i];
            return true;
        }
    }
    return false;
}

RayHit *
SceneQuery::intersect(
    RpkRay *ray,
    const float minimumDistance,
    float *maximumDistance,
    const int hitFlags,
    RayHit *hitStore,
    const RayExclusionSet &exclusions,
    SceneQueryWorkspace *workspace) const
{
    if ( legacyGrid != NULL ) {
        return legacyGrid->gridIntersect(
            ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions, workspace->getLegacyWorkspace());
    }

    SceneFaceRayTest test(*scene, ray, hitFlags, exclusions, hitStore);
    const Ray indexRay = Ray::fromAffine(
        Vector3Dd(ray->position.x(), ray->position.y(), ray->position.z()),
        Vector3Dd(ray->direction.x(), ray->direction.y(), ray->direction.z()));
    const SpatialQueryMode::Value mode = (hitFlags & ANY) != 0
        ? SpatialQueryMode::ANY_HIT : SpatialQueryMode::CLOSEST_HIT;
    const SpatialQueryResult result = index->query(
        indexRay, minimumDistance, *maximumDistance, mode, test, *workspace->getIndexWorkspace());
    if ( !result.hasHit() ) {
        return NULL;
    }
    *maximumDistance = ((float)(result.getParameter()));
    return hitStore;
}

SceneQueryWorkspace::SceneQueryWorkspace(const SceneQuery &query, VisibilityQueryRecorder *inRecorder):
    legacyWorkspace(NULL),
    indexWorkspace(NULL),
    recorder(inRecorder)
{
    if ( query.getLegacyGrid() != NULL ) {
        legacyWorkspace = new VoxelGridWorkspace(query.getLegacyGrid()->getNumberOfItems(), inRecorder);
    } else {
        indexWorkspace = new SpatialIndexWorkspace(query.getIndex()->getWorkspaceSize());
    }
}

SceneQueryWorkspace::~SceneQueryWorkspace() {
    delete legacyWorkspace;
    legacyWorkspace = NULL;
    delete indexWorkspace;
    indexWorkspace = NULL;
}
